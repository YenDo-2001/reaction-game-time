// Owner: Member 4.
#pragma once
#include "../../hal/gpio.hpp"
#include "../../hal/timer.hpp"
namespace board {
lab1::input_pin& button();
lab1::output_pin& heartbeat();
lab1::output_pin& cue();
lab1::output_pin& scl();
lab1::output_pin& sda();
lab2::steady_clock& clock();
}
