// Owner: Member 3.
#include "oled.hpp"
#include "software_i2c.hpp"
#include "../system/board.hpp"
#include "../system/system.hpp"
#include "task.h"
#include <cstdio>
namespace reaction {
void render(oled& screen, view const& v) {
  static constexpr char const* msg[] = {
    "PRESS TO START", "RELEASE BUTTON", "WAIT...", "GO", "", "FALSE START!", "TIMEOUT / OLED ERROR"
  };
  screen.clear();
  screen.text(0, 0, "REACTION GAME");
  char buf[22];
  if (v.screen == state::go) screen.text(42, 20, "GO", 3);
  else if (v.screen == state::result) {
    std::snprintf(buf, sizeof(buf), "%lu MS", static_cast<unsigned long>(v.reaction_ms));
    screen.text(0, 20, buf, 2);
  } else screen.text(0, 20, msg[static_cast<unsigned>(v.screen)]);
  if (v.screen >= state::result) screen.text(0, v.screen == state::result ? 42 : 36, "PRESS TO RETRY");
  std::snprintf(buf, sizeof(buf), "BEST %lu MS", static_cast<unsigned long>(v.best_ms));
  screen.text(0, 56, buf);
}
void DisplayTask(void* context) {
  auto const& channels = *static_cast<task_channels const*>(context);
  software_i2c bus(board::scl(), board::sda(), board::clock(), {50000});
  oled screen(bus);
  board::cue().level(false);
  frame f{};
  for (;;) {
    if (xQueueReceive(channels.frames, &f, pdMS_TO_TICKS(1000)) != pdTRUE) continue;
    bool ok = (bus.ready() && screen.ready()) || (bus.recover() && screen.recover());
    bool is_go = f.data.screen == state::go;
    if (ok && is_go) ok = screen.power(false);
    if (ok) { render(screen, f.data); ok = screen.flush(); }
    if (ok && is_go) ok = screen.power(true);
    auto visible = now_ms();
    board::cue().level(ok && is_go);
    if (is_go) { display_ack ack{f.token, visible, ok}; xQueueOverwrite(channels.acknowledgements, &ack); }
  }
}
}
