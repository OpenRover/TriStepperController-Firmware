// =============================================================================
// Stepper Controller Firmware for the RoverMaster Project
// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
#ifdef TARGET_DEMO

#include "board.h"
#include "esp32-hal.h"
#include "global.h"
#include "motor.h"
#include "utils.h"
#include <Arduino.h>

constexpr double R0 = 60.0;
constexpr double R1 = 10.0;
constexpr double R2 = 9.2;

void setup() {
  try {
    Serial.begin(Global::Config::SERIAL_BAUD_RATE);
    Board::LED::BLUE.init();
    for (unsigned n = 0; n < 50; n++) {
      Board::LED::BLUE.write(LOW);
      delay(50);
      Board::LED::BLUE.write(HIGH);
      delay(50);
    }
    Board::init();
    Motor::init();
    motors[0].configure({
        .invert = false,
        .micro_steps = 32,
        .stall_sensitivity = 40,
        .rms_current = 1000,
    });
    motors[1].configure({
        .invert = false,
        .micro_steps = 32,
        .stall_sensitivity = 40,
        .rms_current = 1000,
    });
    motors[2].configure({
        .invert = false,
        .micro_steps = 32,
        .stall_sensitivity = 40,
        .rms_current = 1000,
    });
    motors[0].enable();
    motors[0].run(+R0, 60_us);
    motors[0].run(-R0 / 2, 60_us);
    motors[2].enable();
    motors[2].run(-R2, 40_us);
    motors[1].enable();
  } catch (const std::exception &e) {
    PANIC("%s", e.what());
  }
}

void loop() {
  motors[0].run(-R0 * 0.7, 60_us);
  motors[0].run(+R0, 60_us);
  motors[0].run(-R0 * 0.3, 60_us);
  delay(1000);
  motors[1].run(+R1 / 2, 60_us);
  delay(1000);
  motors[2].run(+R2, 60_us);
  motors[2].run(-R2, 60_us);
  delay(1000);
  motors[1].run(+R1 / 2, 60_us);
  delay(1000);
}

#endif