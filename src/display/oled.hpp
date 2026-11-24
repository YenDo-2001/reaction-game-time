// Owner: Member 3.
#pragma once
#include "../../hal/i2c.hpp"
namespace reaction {
// Portable SSD1306 128x64 I2C OLED driver.
class oled {
public:
  explicit oled(lab4::i2c& bus);
  [[nodiscard]] bool ready() const { return ready_; }
  bool recover();
  void clear();
  void text(unsigned x, unsigned y, char const* s, unsigned scale = 1);
  bool flush();
  bool power(bool on);
private:
  bool commands(std::span<std::uint8_t const> bytes);
  lab4::i2c& bus_;
  std::uint8_t address_ = 0x3c, pixels_[1024]{};
  bool ready_ = false;
};
}
