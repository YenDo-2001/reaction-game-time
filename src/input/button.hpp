// Owner: Member 1.
#pragma once
#include "../../hal/gpio.hpp"
#include <cstdint>
namespace reaction {
struct button_event { bool pressed; std::uint32_t time_ms; };
// Portable active-low debounced button driver; preserves first-edge timestamp.
class button {
public:
  explicit button(lab1::input_pin& pin) : pin_(pin) {}
  bool poll(std::uint32_t now, button_event& event) {
    bool pressed = !pin_.level();
    if (pressed != candidate_) { candidate_ = pressed; since_ = now; }
    if (candidate_ != stable_ && std::uint32_t(now - since_) >= 20U) {
      stable_ = candidate_; event = {stable_, since_}; return true;
    }
    return false;
  }
private:
  lab1::input_pin& pin_;
  bool candidate_ = false, stable_ = false;
  std::uint32_t since_ = 0;
};
}
