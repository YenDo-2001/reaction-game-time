// Owner: Member 2. Study this task and the drivers in this folder.
#include "game.hpp"
#include "../system/system.hpp"
#include "task.h"
namespace reaction {
void GameTask(void* context) {
  auto const& channels = *static_cast<task_channels const*>(context);
  game engine;
  std::uint32_t token=1, pending_token=0, rng=0x1463507;
  bool pending=false;
  auto publish=[&](view v) { frame f{v,++token}; xQueueOverwrite(channels.frames,&f); return token; };
  publish(engine.current);
  for (;;) {
    auto old=engine.current.screen;
    display_ack ack{};
    if (xQueueReceive(channels.acknowledgements,&ack,0)==pdTRUE && pending && ack.token==pending_token) {
      pending=false;
      if (ack.ok) engine.cue_visible(ack.visible_ms);
      else engine.current.screen=state::timeout;
    }
    button_event e{};
    while (xQueueReceive(channels.buttons,&e,0)==pdTRUE) {
      if (e.pressed) {
        rng ^= e.time_ms; rng ^= rng<<13; rng ^= rng>>17; rng ^= rng<<5;
        engine.press(e.time_ms,2000+rng%3001);
      } else engine.release(now_ms());
    }
    engine.tick(now_ms());
    if (engine.current.screen!=old) {
      // A false start cancels an outstanding display request; stale acks are ignored.
      if (engine.current.screen!=state::go) pending=false;
      // GO was already rendered and timed by DisplayTask. Do not redraw it.
      if (engine.current.screen!=state::go) publish(engine.current);
    }
    if (!pending && engine.due(now_ms())) {
      view request=engine.current; request.screen=state::go;
      pending_token=publish(request); pending=true;
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
}
