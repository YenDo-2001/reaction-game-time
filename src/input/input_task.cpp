// Owner: Member 1. Study this task and the drivers in this folder.
#include "button.hpp"
#include "../system/board.hpp"
#include "../system/system.hpp"
#include "task.h"
namespace reaction {
void InputTask(void* context) {
  auto const& channels = *static_cast<task_channels const*>(context);
  button input(board::button());
  TickType_t next=xTaskGetTickCount();
  for (;;) {
    button_event e{};
    if (input.poll(now_ms(),e)) {
      // Queue capacity covers rapid presses; preserve every validated transition.
      configASSERT(xQueueSend(channels.buttons,&e,0)==pdTRUE);
    }
    vTaskDelayUntil(&next,pdMS_TO_TICKS(1));
  }
}
}
