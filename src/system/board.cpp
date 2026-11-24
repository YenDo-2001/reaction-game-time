// Owner: Member 4.
#include "board.hpp"
#include <ti/driverlib/driverlib.h>
namespace {
// ---------------------------------------------------------------------------
// Register-level helpers shared by the drivers below
// ---------------------------------------------------------------------------

// Every MSPM0 peripheral's PWREN/RSTCTL register takes the same unlock keys
// in its top byte (TRM "Peripheral Power Enable Control").
constexpr std::uint32_t power_enable =
  GPIO_PWREN_KEY_UNLOCK_W | GPIO_PWREN_ENABLE_ENABLE;
constexpr std::uint32_t reset_assert = GPIO_RSTCTL_KEY_UNLOCK_W | //unlocks Reset Control register
                                       GPIO_RSTCTL_RESETSTKYCLR_CLR | //clears sticky reset status
                                       GPIO_RSTCTL_RESETASSERT_ASSERT; //requests peripheral reset
// Peripherals need a few bus cycles after PWREN before they accept writes.
constexpr std::uint32_t power_up_cycles = 32;
constexpr std::uint32_t mcu_clock = 32'000'000;  // SYSOSC, MCLK = ULPCLK

void wait_cycles(std::uint32_t p_cycles)
{
  // Each iteration is at least 4 cycles on a Cortex-M0+.
  for (std::uint32_t count = 0; count < p_cycles / 4 + 1; count++) {
    __NOP() ; //No Operation
  }
}

template<class Gprcm>
void reset_and_power(Gprcm& p_gprcm)
{
  p_gprcm.RSTCTL = reset_assert;
  p_gprcm.PWREN = power_enable;
  wait_cycles(power_up_cycles);
}

template<class Gprcm>
void power_if_needed(Gprcm& p_gprcm)
{
  // GPIO ports are shared by several devices; do not reset an active port.
  if ((p_gprcm.PWREN & GPIO_PWREN_ENABLE_MASK) == 0) {
    reset_and_power(p_gprcm);
  }
}

constexpr std::uint32_t pin_function_gpio = 0x1;  // IOMUX PF value for GPIO

void set_pin_mux(std::uint8_t p_pincm, std::uint32_t p_value)
{
  // PINCMn in the datasheet pin table is PINCM[n - 1] in the register map.
  IOMUX->SECCFG.PINCM[p_pincm - 1U] = p_value;
}

constexpr std::uint32_t pull_bits(lab1::pin_resistor p_resistor)
{
  switch (p_resistor) {
    case lab1::pin_resistor::pull_down:
      return IOMUX_PINCM_PIPD_ENABLE;
    case lab1::pin_resistor::pull_up:
      return IOMUX_PINCM_PIPU_ENABLE;
    case lab1::pin_resistor::none:
    default:
      return 0;
  }
}

/// RAII interrupt mask, safe to nest and to use from an ISR.
class interrupt_lock
{
public:
  interrupt_lock()
    : m_primask(__get_PRIMASK())
  {
    __disable_irq();
  }
  interrupt_lock(interrupt_lock const&) = delete;
  interrupt_lock& operator=(interrupt_lock const&) = delete;
  ~interrupt_lock()
  {
    __set_PRIMASK(m_primask);
  }

private:
  std::uint32_t m_primask;
};


// ---------------------------------------------------------------------------
// GPIO
// ---------------------------------------------------------------------------

enum class gpio_port : std::uint8_t
{
  a,
  b,
};

/// A pin's GPIO port/bit plus its IOMUX PINCM number from the datasheet.
struct pin_location
{
  gpio_port port;
  std::uint8_t pin;
  std::uint8_t pincm;
};

// LP-MSPM0G3507 RGB LED channels used by this application are active-high.
constexpr pin_location led_red{ gpio_port::b, 26, 57 };
constexpr pin_location led_green{ gpio_port::b, 27, 58 };

GPIO_Regs* port_registers(gpio_port p_port)
{
  return p_port == gpio_port::a ? GPIOA : GPIOB;
}

class mspm0_input_pin final : public lab1::input_pin
{
public:
  mspm0_input_pin(pin_location p_location, settings const& p_settings)
    : m_location(p_location)
  {
    power_if_needed(port_registers(m_location.port)->GPRCM);
    (void)driver_configure(p_settings);
  }

private:
  bool driver_configure(settings const& p_settings) override
  {
    port_registers(m_location.port)->DOECLR31_0 = mask();
    set_pin_mux(m_location.pincm,
                IOMUX_PINCM_PC_CONNECTED | IOMUX_PINCM_INENA_ENABLE |
                  pull_bits(p_settings.resistor) | pin_function_gpio);
    return true;
  }

  bool driver_level() override
  {
    return (port_registers(m_location.port)->DIN31_0 & mask()) != 0;
  }

  [[nodiscard]] std::uint32_t mask() const
  {
    return 1UL << m_location.pin;
  }

  pin_location m_location;
};

class mspm0_output_pin final : public lab1::output_pin
{
public:
  mspm0_output_pin(pin_location p_location, settings const& p_settings,
                  bool p_initial_level)
    : m_location(p_location)
  {
    power_if_needed(port_registers(m_location.port)->GPRCM);
    // Set the latch before enabling output: LEDs start off, I2C lines released.
    driver_level(p_initial_level);
    (void)driver_configure(p_settings);
  }

private:
  bool driver_configure(settings const& p_settings) override
  {
    auto const open_drain =
      p_settings.open_drain ? IOMUX_PINCM_HIZ1_ENABLE : 0U;
    // INENA lets level() read the pin back from hardware.
    set_pin_mux(m_location.pincm,
                IOMUX_PINCM_PC_CONNECTED | IOMUX_PINCM_INENA_ENABLE |
                  open_drain | pull_bits(p_settings.resistor) |
                  pin_function_gpio);
    port_registers(m_location.port)->DOESET31_0 = mask();
    return true;
  }

  void driver_level(bool p_high) override
  {
    auto* const port = port_registers(m_location.port);
    if (p_high) {
      port->DOUTSET31_0 = mask();
    } else {
      port->DOUTCLR31_0 = mask();
    }
  }

  bool driver_level() override
  {
    return (port_registers(m_location.port)->DIN31_0 & mask()) != 0;
  }

  [[nodiscard]] std::uint32_t mask() const
  {
    return 1UL << m_location.pin;
  }

  pin_location m_location;
};

// ---------------------------------------------------------------------------
// Steady clock: TIMG12 (32-bit) counting BUSCLK, extended to 64 bits in
// software by counting wraps in its zero-event interrupt.
// ---------------------------------------------------------------------------

class timg12_steady_clock;
timg12_steady_clock* active_clock = nullptr;

class timg12_steady_clock final : public lab2::steady_clock
{
public:
  timg12_steady_clock()
  {
    reset_and_power(TIMG12->GPRCM);
    TIMG12->CLKSEL = GPTIMER_CLKSEL_BUSCLK_SEL_ENABLE;
    TIMG12->CLKDIV = GPTIMER_CLKDIV_RATIO_DIV_BY_1;
    TIMG12->COMMONREGS.CPS = 0;  // prescale by 1
    TIMG12->COMMONREGS.CCLKCTL = GPTIMER_CCLKCTL_CLKEN_ENABLED;
    TIMG12->COUNTERREGS.LOAD = 0xFFFF'FFFFU;
    TIMG12->COUNTERREGS.CTRCTL =
      GPTIMER_CTRCTL_CM_UP | GPTIMER_CTRCTL_REPEAT_REPEAT_1 |
      GPTIMER_CTRCTL_CVAE_ZEROVAL | GPTIMER_CTRCTL_EN_ENABLED;
    // Zero event == the counter wrapping from 0xFFFFFFFF back to 0.
    TIMG12->CPU_INT.ICLR = GPTIMER_CPU_INT_IMASK_Z_SET;
    active_clock = this;
    TIMG12->CPU_INT.IMASK = GPTIMER_CPU_INT_IMASK_Z_SET;
    NVIC_ClearPendingIRQ(TIMG12_INT_IRQn);
    NVIC_EnableIRQ(TIMG12_INT_IRQn);
  }
  timg12_steady_clock(timg12_steady_clock const&) = delete;
  timg12_steady_clock& operator=(timg12_steady_clock const&) = delete;

  void handle_interrupt()
  {
    if ((TIMG12->CPU_INT.MIS & GPTIMER_CPU_INT_IMASK_Z_SET) != 0) {
      TIMG12->CPU_INT.ICLR = GPTIMER_CPU_INT_IMASK_Z_SET;
      m_wraps = m_wraps + 1;
    }
  }

private:
  std::uint32_t driver_frequency() override
  {
    return mcu_clock;
  }

  std::uint64_t driver_uptime() override
  {
    interrupt_lock const lock;
    // With interrupts masked, a wrap can still happen between reading the
    // counter and checking the flag. Re-read the counter if the flag
    // appears in between so the pair is always consistent.
    auto wrap_pending =
      (TIMG12->CPU_INT.RIS & GPTIMER_CPU_INT_IMASK_Z_SET) != 0;
    std::uint32_t counter = TIMG12->COUNTERREGS.CTR;
    if (!wrap_pending &&
        (TIMG12->CPU_INT.RIS & GPTIMER_CPU_INT_IMASK_Z_SET) != 0) {
      wrap_pending = true;
      counter = TIMG12->COUNTERREGS.CTR;
    }
    std::uint64_t const wraps = m_wraps + (wrap_pending ? 1U : 0U);
    return (wraps << 32U) | counter;
  }

  std::uint32_t volatile m_wraps = 0;
};

}

extern "C" void TIMG12_IRQHandler() { if (active_clock) active_clock->handle_interrupt(); }
namespace board {
lab1::input_pin& button() {
  static mspm0_input_pin pin({gpio_port::a,12,34},{lab1::pin_resistor::pull_up});
  return pin;
}
lab1::output_pin& heartbeat() {
  static mspm0_output_pin pin(led_red,{},false); return pin;
}
lab1::output_pin& cue() {
  static mspm0_output_pin pin(led_green,{},false); return pin;
}
lab1::output_pin& scl() {
  static mspm0_output_pin pin({gpio_port::b,2,15},{lab1::pin_resistor::none,true},true);
  return pin;
}
lab1::output_pin& sda() {
  static mspm0_output_pin pin({gpio_port::b,3,16},{lab1::pin_resistor::none,true},true);
  return pin;
}
lab2::steady_clock& clock() { static timg12_steady_clock timer; return timer; }
}
