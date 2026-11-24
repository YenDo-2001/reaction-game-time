// Owner: Member 2.
#pragma once
#include <cstdint>
namespace reaction {
enum class state : std::uint8_t { ready, release, waiting, go, result, early, timeout };
struct view { state screen = state::ready; std::uint32_t reaction_ms = 0, best_ms = 0; };
// Pure state machine; unsigned subtraction handles 32-bit ms wraparound.
class game {
public:
  view current{};
  void press(std::uint32_t edge, std::uint32_t delay) {
    if (current.screen == state::waiting) { current.screen = state::early; return; }
    if (current.screen == state::release) return;
    if (current.screen != state::go) { current.screen = state::release; delay_ = delay; return; }
    std::uint32_t dt = edge - go_at_;
    if (dt > 0x7fffffffU) current.screen = state::early;
    else if (dt >= 10000U) current.screen = state::timeout;
    else {
      current.reaction_ms = dt;
      if (!current.best_ms || dt < current.best_ms) current.best_ms = dt;
      current.screen = state::result;
    }
  }
  void release(std::uint32_t now) {
    if (current.screen == state::release) { wait_at_ = now; current.screen = state::waiting; }
  }
  bool due(std::uint32_t now) const {
    return current.screen == state::waiting && std::uint32_t(now - wait_at_) >= delay_;
  }
  void cue_visible(std::uint32_t now) { go_at_ = now; current.screen = state::go; }
  void tick(std::uint32_t now) {
    if (current.screen == state::go && std::uint32_t(now - go_at_) >= 10025U) current.screen = state::timeout;
  }
private:
  std::uint32_t wait_at_ = 0, go_at_ = 0, delay_ = 2000;
};
}
