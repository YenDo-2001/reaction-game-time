// Owner: Member 3. Study this task and the drivers in this folder.
#include "oled.hpp"
#include "software_i2c.hpp"
#include "../system/board.hpp"
#include <cstdio>
#include "../system/system.hpp"
#include "task.h"
namespace reaction {
void render(oled& screen,view const& v) {
  screen.clear(); screen.text(0,0,"REACTION GAME");
  char number[22];
  switch (v.screen) {
    case state::ready: screen.text(0,20,"PRESS TO START"); break;
    case state::release: screen.text(0,20,"RELEASE BUTTON"); break;
    case state::waiting: screen.text(0,20,"WAIT..."); break;
    case state::go: screen.text(42,20,"GO",3); break;
    case state::result:
      std::snprintf(number,sizeof(number),"%lu MS",static_cast<unsigned long>(v.reaction_ms));
      screen.text(0,20,number,2); screen.text(0,42,"PRESS TO RETRY"); break;
    case state::early: screen.text(0,20,"FALSE START!"); screen.text(0,36,"PRESS TO RETRY"); break;
    case state::timeout: screen.text(0,20,"TIMEOUT / OLED ERROR"); screen.text(0,36,"PRESS TO RETRY"); break;
  }
  std::snprintf(number,sizeof(number),"BEST %lu MS",static_cast<unsigned long>(v.best_ms));
  screen.text(0,56,number);
}
void DisplayTask(void* context) {
  auto const& channels = *static_cast<task_channels const*>(context);
  software_i2c bus(board::scl(),board::sda(),board::clock(),{50000});
  oled screen(bus);
  board::cue().level(false);
  frame f{};
  for (;;) {
    if (xQueueReceive(channels.frames,&f,pdMS_TO_TICKS(1000))!=pdTRUE) continue;
    bool ok=bus.ready() && screen.ready();
    // Recovery is for a failed device, not a required second initialization step.
    if (!ok) ok=bus.recover() && screen.recover();
    if (ok && f.data.screen==state::go) ok=screen.power(false);
    if (ok) { render(screen,f.data); ok=screen.flush(); }
    if (ok && f.data.screen==state::go) ok=screen.power(true);
    auto visible=now_ms();
    board::cue().level(ok && f.data.screen==state::go);
    if (f.data.screen==state::go) {
      display_ack ack{f.token,visible,ok}; xQueueOverwrite(channels.acknowledgements,&ack);
    }
  }
}
}
