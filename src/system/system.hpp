// Owner: Member 4. Task contracts and queue handles; other members consume this API.
#pragma once
#include "FreeRTOS.h"
#include "queue.h"
#include "../input/button.hpp"
#include "../game/game.hpp"
namespace reaction {
struct frame { view data; std::uint32_t token; };
struct display_ack { std::uint32_t token, visible_ms; bool ok; };
struct task_channels {
  QueueHandle_t buttons;
  QueueHandle_t frames;
  QueueHandle_t acknowledgements;
};
std::uint32_t now_ms();
void InputTask(void*);
void GameTask(void*);
void DisplayTask(void*);
void HeartbeatTask(void*);
}
