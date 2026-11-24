// Owner: Member 4. System integration, FreeRTOS startup and heartbeat.
#include "system.hpp"
#include "board.hpp"
#include "task.h"
namespace {
using namespace reaction;
StaticQueue_t button_control, frame_control, ack_control;
alignas(4) std::uint8_t button_storage[16*sizeof(button_event)];
alignas(4) std::uint8_t frame_storage[sizeof(frame)], ack_storage[sizeof(display_ack)];
task_channels channels{};
StaticTask_t tcbs[4];
StackType_t stacks[4][512];
}
namespace reaction {
std::uint32_t now_ms() { return std::uint32_t(board::clock().uptime()/(board::clock().frequency()/1000)); }
void HeartbeatTask(void*) {
  auto& led=board::heartbeat();
  bool on=false;
  for (;;) { on=!on; led.level(on); vTaskDelay(pdMS_TO_TICKS(500)); }
}
}
extern "C" void SYSCFG_DL_init(void);
int main() {
  using namespace reaction;
  SYSCFG_DL_init();
  (void)board::clock();
  channels.buttons=xQueueCreateStatic(16,sizeof(reaction::button_event),button_storage,&button_control);
  channels.frames=xQueueCreateStatic(1,sizeof(frame),frame_storage,&frame_control);
  channels.acknowledgements=xQueueCreateStatic(1,sizeof(display_ack),ack_storage,&ack_control);
  configASSERT(channels.buttons && channels.frames && channels.acknowledgements);
  configASSERT(xTaskCreateStatic(InputTask,"InputTask",512,&channels,3,stacks[0],&tcbs[0]));
  configASSERT(xTaskCreateStatic(GameTask,"GameTask",512,&channels,2,stacks[1],&tcbs[1]));
  configASSERT(xTaskCreateStatic(DisplayTask,"DisplayTask",512,&channels,1,stacks[2],&tcbs[2]));
  configASSERT(xTaskCreateStatic(HeartbeatTask,"HeartbeatTask",512,&channels,1,stacks[3],&tcbs[3]));
  vTaskStartScheduler();
  for (;;) {}
}
