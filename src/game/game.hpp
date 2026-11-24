// Owner: Member 2.
#pragma once
#include <cstdint>
namespace reaction {
enum class state : std::uint8_t { ready, release, waiting, go, result, early, timeout };
struct view { state screen=state::ready; std::uint32_t reaction_ms=0, best_ms=0; };
// Pure state machine. All intervals are <2^31 ms; unsigned subtraction handles wrap.
class game {
public:
  view current{};
  void press(std::uint32_t edge, std::uint32_t delay) {
    switch (current.screen) {
      case state::ready: case state::result: case state::early: case state::timeout:
        current.screen=state::release; delay_=delay; break;
      case state::waiting: current.screen=state::early; break;
      case state::go:
        // A debounced press may arrive after GO although its first edge was earlier.
        if (std::uint32_t(edge-go_at_) > 0x7fffffffU) current.screen=state::early;
        else if (std::uint32_t(edge-go_at_)>=10000) current.screen=state::timeout;
        else {
          current.reaction_ms=edge-go_at_;
          if (!current.best_ms || current.reaction_ms<current.best_ms)
            current.best_ms=current.reaction_ms;
          current.screen=state::result;
        }
        break;
      default: break;
    }
  }
  void release(std::uint32_t now) {
    if (current.screen==state::release) { wait_at_=now; current.screen=state::waiting; }
  }
  bool due(std::uint32_t now) const {
    return current.screen==state::waiting && std::uint32_t(now-wait_at_)>=delay_;
  }
  // DisplayTask acknowledges GO only after the GO pixels have been transmitted.
  void cue_visible(std::uint32_t now) { go_at_=now; current.screen=state::go; }
  void tick(std::uint32_t now) {
    // Allow the 20 ms debounce to deliver a press just before the deadline.
    if (current.screen==state::go && std::uint32_t(now-go_at_)>=10025)
      current.screen=state::timeout;
  }
private:
  std::uint32_t wait_at_=0, go_at_=0, delay_=2000;
};
}
