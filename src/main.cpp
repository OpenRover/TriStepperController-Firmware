// =============================================================================
// Stepper Controller Firmware for the RoverMaster Project
// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
#ifndef TARGET_DEMO

#include "agent.h"
#include "board.h"
#include "esp32-hal.h"
#include "global.h"
#include "motor.h"
#include "scheduler.h"
#include "utils.h"
#include <Arduino.h>

class WatchDog : public Scheduler::Task {
public:
  WatchDog() : Scheduler::Task(Scheduler::Task::Recurrent{100_ms}) {}
  unsigned report_counter = 0;
  void tick(Scheduler::Micros now) override {
    Board::LED::BUILTIN.write(!Board::LED::BUILTIN.read());
    if (++report_counter >= 50) {
      report_counter = 0;
      unsigned long duration = now - Global::scheduler.perf.since;
      LOG("[PERF] Util=%.2f%%, Freq=%.2f tick/ms",
          Global::scheduler.perf.utilization(duration) * 100.0,
          Global::scheduler.perf.frequency(duration));
      Global::scheduler.perf.reset();
    }
  }
} watchdog;

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
    auto &tasks = Global::scheduler.tasks;
    tasks.clear().add(agent).add(watchdog);
    for (auto &m : motors)
      tasks.add(m).add(m.subtask);
    Board::LED::BLUE.init();
    Global::scheduler.loop();
  } catch (const std::exception &e) {
    PANIC("%s", e.what());
  }
}

void loop() {}

#endif