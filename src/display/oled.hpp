// Owner: Member 3.
#pragma once
#include "../../hal/i2c.hpp"
#include <array>
namespace reaction {
// Portable SSD1306 128x64 device driver; controller/model still needs confirmation.
class oled {
public:
  explicit oled(lab4::i2c& bus);
  [[nodiscard]] bool ready() const { return ready_; }
  bool recover();
  void clear();
  void text(unsigned x,unsigned y,char const* s,unsigned scale=1);
  bool flush();
  bool power(bool on);
private:
  bool initialize();
  bool commands(std::span<std::uint8_t const> bytes);
  lab4::i2c& bus_;
  std::uint8_t address_=0x3c;
  std::array<std::uint8_t,1024> pixels_{};
  bool ready_ = false;
};
}
