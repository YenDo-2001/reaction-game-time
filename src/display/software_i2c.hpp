// Owner: Member 3.
#pragma once
#include "../../hal/i2c.hpp"
#include "../../hal/gpio.hpp"
#include "../../hal/timer.hpp"
namespace reaction {
// Portable, bounded, open-drain I2C master, single owner DisplayTask.
// External pullups to 3.3V (typically on OLED module) are required.
class software_i2c final : public lab4::i2c {
public:
  software_i2c(lab1::output_pin& scl, lab1::output_pin& sda,
               lab2::steady_clock& clock, settings const& config = settings{});
  [[nodiscard]] bool ready() const { return ready_; }
  bool recover();
private:
  bool configure_bus(settings const&);
  bool driver_configure(settings const& s) override;
  bool driver_transaction(std::uint8_t, std::span<std::uint8_t const>,
                          std::span<std::uint8_t>) override;
  void pause();
  bool high();
  bool start();
  void stop();
  bool write(std::uint8_t);
  bool read(std::uint8_t&, bool);
  lab1::output_pin &scl_, &sda_;
  lab2::steady_clock& clock_;
  std::uint32_t half_ticks_ = 320;
  settings settings_;
  bool ready_ = false;
};
}
