// Owner: Member 4.
#include "system.hpp"
#include "board.hpp"
#include "task.h"
namespace {
using namespace reaction;
StaticQueue_t q_ctrl[3];
alignas(4) std::uint8_t button_buf[16 * sizeof(button_event)], frame_buf[sizeof(frame)], ack_buf[sizeof(display_ack)];
task_channels channels{};
StaticTask_t tcbs[4];
StackType_t stacks[4][512];
}
namespace reaction {
std::uint32_t now_ms() { return std::uint32_t(board::clock().uptime() / (board::clock().frequency() / 1000U)); }
void HeartbeatTask(void*) {
  auto& led = board::heartbeat();
  for (bool on = false;;) { led.level(on = !on); vTaskDelay(pdMS_TO_TICKS(500)); }
}
}
extern "C" void SYSCFG_DL_init(void);
int main() {
  using namespace reaction;
  SYSCFG_DL_init();
  (void)board::clock();
  channels.buttons = xQueueCreateStatic(16, sizeof(button_event), button_buf, &q_ctrl[0]);
  channels.frames = xQueueCreateStatic(1, sizeof(frame), frame_buf, &q_ctrl[1]);
  channels.acknowledgements = xQueueCreateStatic(1, sizeof(display_ack), ack_buf, &q_ctrl[2]);
  configASSERT(channels.buttons && channels.frames && channels.acknowledgements);
  struct { TaskFunction_t fn; char const* name; UBaseType_t prio; } const tasks[] = {
    {InputTask, "InputTask", 3}, {GameTask, "GameTask", 2},
    {DisplayTask, "DisplayTask", 1}, {HeartbeatTask, "HeartbeatTask", 1}
  };
  for (int i = 0; i < 4; ++i)
    configASSERT(xTaskCreateStatic(tasks[i].fn, tasks[i].name, 512, &channels, tasks[i].prio, stacks[i], &tcbs[i]));
  vTaskStartScheduler();
  for (;;) {}
}
