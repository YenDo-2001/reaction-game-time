// Owner: Member 4.
#include "board.hpp"
#include <ti/driverlib/driverlib.h>

namespace {
volatile std::uint32_t timg12_wraps = 0;

template<class Gprcm>
void power_on(Gprcm& g, bool reset = false) {
  if (!reset && (g.PWREN & GPIO_PWREN_ENABLE_MASK)) return;
  g.RSTCTL = GPIO_RSTCTL_KEY_UNLOCK_W | GPIO_RSTCTL_RESETSTKYCLR_CLR | GPIO_RSTCTL_RESETASSERT_ASSERT;
  g.PWREN = GPIO_PWREN_KEY_UNLOCK_W | GPIO_PWREN_ENABLE_ENABLE;
  for (int i = 0; i < 9; ++i) __NOP();
}

struct gpio_pin {
  GPIO_Regs* port;
  std::uint32_t mask;
  std::uint8_t pincm;
  bool config(lab1::pin_resistor r, bool out, bool od = false) const {
    std::uint32_t pull = r == lab1::pin_resistor::pull_up ? IOMUX_PINCM_PIPU_ENABLE
                       : r == lab1::pin_resistor::pull_down ? IOMUX_PINCM_PIPD_ENABLE : 0U;
    IOMUX->SECCFG.PINCM[pincm - 1U] = IOMUX_PINCM_PC_CONNECTED | IOMUX_PINCM_INENA_ENABLE |
                                      (od ? IOMUX_PINCM_HIZ1_ENABLE : 0U) | pull | 0x1U;
    if (out) port->DOESET31_0 = mask; else port->DOECLR31_0 = mask;
    return true;
  }
  bool read() const { return (port->DIN31_0 & mask) != 0; }
  void write(bool hi) const { if (hi) port->DOUTSET31_0 = mask; else port->DOUTCLR31_0 = mask; }
};

class mspm0_input_pin final : public lab1::input_pin {
public:
  mspm0_input_pin(GPIO_Regs* p, std::uint8_t bit, std::uint8_t cm, settings const& s)
    : pin_{p, 1UL << bit, cm} { power_on(p->GPRCM); (void)driver_configure(s); }
private:
  bool driver_configure(settings const& s) override { return pin_.config(s.resistor, false); }
  bool driver_level() override { return pin_.read(); }
  gpio_pin pin_;
};

class mspm0_output_pin final : public lab1::output_pin {
public:
  mspm0_output_pin(GPIO_Regs* p, std::uint8_t bit, std::uint8_t cm, settings const& s, bool hi)
    : pin_{p, 1UL << bit, cm} { power_on(p->GPRCM); pin_.write(hi); (void)driver_configure(s); }
private:
  bool driver_configure(settings const& s) override { return pin_.config(s.resistor, true, s.open_drain); }
  void driver_level(bool hi) override { pin_.write(hi); }
  bool driver_level() override { return pin_.read(); }
  gpio_pin pin_;
};

class timg12_steady_clock final : public lab2::steady_clock {
public:
  timg12_steady_clock() {
    power_on(TIMG12->GPRCM, true);
    TIMG12->CLKSEL = GPTIMER_CLKSEL_BUSCLK_SEL_ENABLE;
    TIMG12->CLKDIV = GPTIMER_CLKDIV_RATIO_DIV_BY_1;
    TIMG12->COMMONREGS.CPS = 0;
    TIMG12->COMMONREGS.CCLKCTL = GPTIMER_CCLKCTL_CLKEN_ENABLED;
    TIMG12->COUNTERREGS.LOAD = 0xFFFF'FFFFU;
    TIMG12->COUNTERREGS.CTRCTL = GPTIMER_CTRCTL_CM_UP | GPTIMER_CTRCTL_REPEAT_REPEAT_1 |
                                 GPTIMER_CTRCTL_CVAE_ZEROVAL | GPTIMER_CTRCTL_EN_ENABLED;
    TIMG12->CPU_INT.ICLR = GPTIMER_CPU_INT_IMASK_Z_SET;
    TIMG12->CPU_INT.IMASK = GPTIMER_CPU_INT_IMASK_Z_SET;
    NVIC_ClearPendingIRQ(TIMG12_INT_IRQn);
    NVIC_EnableIRQ(TIMG12_INT_IRQn);
  }
private:
  std::uint32_t driver_frequency() override { return 32'000'000U; }
  std::uint64_t driver_uptime() override {
    std::uint32_t primask = __get_PRIMASK();
    __disable_irq();
    bool wrap = (TIMG12->CPU_INT.RIS & GPTIMER_CPU_INT_IMASK_Z_SET) != 0;
    std::uint32_t ctr = TIMG12->COUNTERREGS.CTR;
    if (!wrap && (TIMG12->CPU_INT.RIS & GPTIMER_CPU_INT_IMASK_Z_SET)) { wrap = true; ctr = TIMG12->COUNTERREGS.CTR; }
    std::uint64_t wraps = timg12_wraps + (wrap ? 1U : 0U);
    __set_PRIMASK(primask);
    return (wraps << 32U) | ctr;
  }
};
}  // namespace

extern "C" void TIMG12_IRQHandler() {
  if (TIMG12->CPU_INT.MIS & GPTIMER_CPU_INT_IMASK_Z_SET) {
    TIMG12->CPU_INT.ICLR = GPTIMER_CPU_INT_IMASK_Z_SET;
    timg12_wraps = timg12_wraps + 1U;
  }
}

namespace board {
lab1::input_pin& button()     { static mspm0_input_pin  p(GPIOA, 12, 34, {lab1::pin_resistor::pull_up}); return p; }
lab1::output_pin& heartbeat() { static mspm0_output_pin p(GPIOB, 26, 57, {}, false); return p; }
lab1::output_pin& cue()       { static mspm0_output_pin p(GPIOB, 27, 58, {}, false); return p; }
lab1::output_pin& scl()       { static mspm0_output_pin p(GPIOB,  2, 15, {lab1::pin_resistor::none, true}, true); return p; }
lab1::output_pin& sda()       { static mspm0_output_pin p(GPIOB,  3, 16, {lab1::pin_resistor::none, true}, true); return p; }
lab2::steady_clock& clock()   { static timg12_steady_clock c; return c; }
}
