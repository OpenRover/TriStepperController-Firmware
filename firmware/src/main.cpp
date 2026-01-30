// =============================================================================
// Stepper Controller Firmware for the RoverMaster Project
// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
#include "agent.h"
#include "board.h"
#include "esp32-hal.h"
#include "global.h"
#include "motor.h"
#include "scheduler.h"
#include "utils.h"
#include <Arduino.h>

class Blinker : public Scheduler::Task {
public:
  Blinker() : Scheduler::Task(Scheduler::Task::Recurrent{100_ms}) {}
  void tick(Scheduler::Micros now) override {
    Board::LED::BUILTIN.write(!Board::LED::BUILTIN.read());
  }
} blinker;

class PerfLogger : public Scheduler::Task {
public:
  PerfLogger() : Scheduler::Task(Scheduler::Task::Recurrent{10000_ms}) {}
  void tick(Scheduler::Micros now) override {
    unsigned long duration = now - Global::scheduler.perf.since;
    LOG("[PERF] Util=%.2f%%, Freq=%.2f tick/ms",
        Global::scheduler.perf.utilization(duration) * 100.0,
        Global::scheduler.perf.frequency(duration));
    Global::scheduler.perf.reset();
  }
} perf;

class PosSync : public Scheduler::Task {
  static char buffer[256];

public:
  PosSync() : Scheduler::Task(Scheduler::Task::Recurrent{16_ms}) {}
  void tick(Scheduler::Micros now) override {
    if (!Board::Drv::is_enabled())
      return;
    auto len = snprintf(buffer, sizeof(buffer), "POS 0=%ld 1=%ld 2=%ld",
                        motors[0].state.position, motors[1].state.position,
                        motors[2].state.position);
    Global::tx.write_frame(Protocol::Method::SYN, Protocol::Property::NA,
                           buffer, len);
    Global::tx.encode_frame();
    Global::tx.send_frame();
  }
} possync;

char PosSync::buffer[256];

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
    tasks.clear().add(agent).add(blinker).add(perf).add(possync);
    for (auto &m : motors)
      tasks.add(m).add(m.subtask);
    Board::LED::BLUE.init();
    Global::scheduler.loop();
  } catch (const std::exception &e) {
    PANIC("%s", e.what());
  }
}

void loop() {}
