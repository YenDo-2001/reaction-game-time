// Owner: Member 2.
#include "game.hpp"
#include "../system/system.hpp"
#include "task.h"
namespace reaction {
void GameTask(void* context) {
  auto const& ch = *static_cast<task_channels const*>(context);
  game engine;
  std::uint32_t token = 1, pending_token = 0, rng = 0x1463507U;
  bool pending = false;
  auto publish = [&](view v) { frame f{v, ++token}; xQueueOverwrite(ch.frames, &f); return token; };
  publish(engine.current);
  for (;;) {
    auto old = engine.current.screen;
    display_ack ack{};
    if (xQueueReceive(ch.acknowledgements, &ack, 0) == pdTRUE && pending && ack.token == pending_token) {
      pending = false;
      if (ack.ok) engine.cue_visible(ack.visible_ms);
      else engine.current.screen = state::timeout;
    }
    button_event e{};
    while (xQueueReceive(ch.buttons, &e, 0) == pdTRUE) {
      if (e.pressed) {
        rng ^= e.time_ms; rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
        engine.press(e.time_ms, 2000U + rng % 3001U);
      } else engine.release(now_ms());
    }
    engine.tick(now_ms());
    if (engine.current.screen != old && engine.current.screen != state::go) {
      pending = false;
      publish(engine.current);
    }
    if (!pending && engine.due(now_ms())) {
      view req = engine.current; req.screen = state::go;
      pending_token = publish(req); pending = true;
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
}
