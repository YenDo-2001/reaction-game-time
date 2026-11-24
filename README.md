# Reaction Time Game

CMPE 146 embedded reaction-time game for one TI LP-MSPM0G3507 LaunchPad,
using FreeRTOS, an external push button, and an SSD1306-compatible 128x64
I2C OLED display. The current game uses a visual GO cue.

## Play

1. Press and release the external button to start.
2. Wait for a pseudorandom delay of 2-5 seconds.
3. When GO appears, press once as quickly as possible.
4. Read the reaction time in milliseconds. Lower is better.
5. Press again to retry. Pressing before GO gives FALSE START.

The response limit is 10 seconds. BEST is stored in RAM and clears on reset
or power loss. Timing is referenced to completion of the OLED display-on
command; it is not optically calibrated.

## Hardware connections

Disconnect USB before rewiring. Confirm the actual module's voltage rating
and printed pin labels. OLED pin order varies between modules.

| Signal | LaunchPad connection |
| --- | --- |
| OLED VCC | 3V3, if the module supports 3.3V |
| OLED GND | GND |
| OLED SCL | PB2 (BoosterPack pin 9) |
| OLED SDA | PB3 (BoosterPack pin 10) |
| Button contact A | PA12 (BoosterPack pin 32) |
| Button contact B | GND |

The button is active-low with an internal pull-up. SDA/SCL require external
pull-ups to 3.3V, usually present on the OLED breakout. I2C is implemented in
software using open-drain GPIO, not the I2C1 hardware peripheral. The driver
tries OLED addresses 0x3C and 0x3D. An SH1106 or a 128x32 display needs changes.

Onboard red LED PB26 is the heartbeat; green PB27 is an auxiliary GO cue.

## Source ownership

| Member | Responsibility | Files |
| --- | --- | --- |
| 1 | Input and debounce | src/input/input_task.cpp, button.hpp |
| 2 | Game logic and scoring | src/game/game_task.cpp, game.hpp |
| 3 | OLED and software I2C | src/display/display_task.cpp, oled.hpp/.cpp, software_i2c.hpp/.cpp |
| 4 | System startup and hardware | src/system/system.cpp, system.hpp, board.hpp/.cpp; tests and scripts |

Drivers, task implementations and system setup remain in separate modules.
Each file has one owner. The system header defines messages, task declarations
and queue handles consumed by the tasks.

## Architecture

- InputTask (priority 3): polls every 1 ms and debounces for 20 ms. Events
  preserve the beginning of the accepted stable interval.
- GameTask (priority 2): owns game state, pseudorandom waiting and scoring.
- DisplayTask (priority 1): exclusively owns OLED/I2C and acknowledges GO
  with its timestamp and a request token.
- HeartbeatTask (priority 1): toggles the red LED every 500 ms.

The buttons queue carries press/release events. Two one-item mailboxes carry
the latest frame and the GO acknowledgement. Tasks and queues use static
allocation. TIMG12 provides an overflow-extended steady clock.

GPIO, I2C and OLED construction performs initialization immediately.
ready() reports success; recover() retries after a device fault. MCU register
access is isolated in src/system/board.cpp. The portable drivers consume the
class interfaces under hal/.

## Build, test and flash

Install the TI MSPM0 SDK and TI ARM Clang toolchain. Default script paths use:

- MSPM0 SDK 2.06.00.05: C:\ti\mspm0_sdk_2_06_00_05
- TI ARM Clang 5.1.1.LTS under CCS 21.0.1
- MSYS2 UCRT64 g++ for host tests
- CCS DSLite for programming through the onboard XDS110 debugger

All paths can be overridden using script parameters. The SDK is an external
dependency; the build does not use another project checkout.

From PowerShell in this repository:

```powershell
powershell -ExecutionPolicy Bypass -File .\test.ps1
powershell -ExecutionPolicy Bypass -File .\build.ps1
# With one board connected, explicitly program and run:
powershell -ExecutionPolicy Bypass -File .\flash.ps1
```

Build output is build/reaction_game.out. build.ps1 and test.ps1 do not flash.
Alternatively use CCS with targetConfigs/MSPM0G3507.ccxml to load that image.
The pre-generated clock/debug configuration and its SysConfig input are
included under config/.

Host tests cover game behavior, debounce, wraparound, driver construction,
OLED fallback addressing and recovery, and I2C NACK/stuck-line/configuration
errors. Physical wiring, display behavior, standalone boot and sustained demo
operation must also be checked on hardware.

## Included dependencies

The hal/ interfaces and third_party/span compatibility header retain their
original headers. TI-generated configuration/startup files and the FreeRTOS
configuration are included; SDK implementation files are supplied by the
separately installed TI SDK. Preserve existing copyright/license notices.
