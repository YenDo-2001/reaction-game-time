// Owner: Member 1.
#include "button.hpp"
#include "../system/board.hpp"
#include "../system/system.hpp"
#include "task.h"
namespace reaction {
void InputTask(void* context) {
  auto const& ch = *static_cast<task_channels const*>(context);
  button input(board::button());
  TickType_t next = xTaskGetTickCount();
  for (;;) {
    button_event e{};
    if (input.poll(now_ms(), e)) (void)xQueueSend(ch.buttons, &e, 0);
    vTaskDelayUntil(&next, pdMS_TO_TICKS(1));
  }
}
}
