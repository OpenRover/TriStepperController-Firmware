// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
#pragma once

#include <cstdint>

#include <TMCStepper.h>

#include "board.h"
#include "duration_literals.h"
#include "global.h"
#include "protocol-header.h"
#include "protocol-impl.h"
#include "ring-buffer.h"

void motorTick();

namespace Motor {
using Sequence = Protocol::Sequence;

typedef struct Command {
  Sequence seq;
  Steps steps;
  Interval interval;
} Command;

class Motor {
public:
  const Board::Pin &step, &dir, &diag;
  const uint8_t addr;
  TMC2209Stepper driver;

  inline Motor(Board::Drv &drv, uint8_t addr)
      : driver(TMC2209Stepper(reinterpret_cast<Stream *>(&Board::Drv::serial),
                              0.11f, addr)),
        step(drv.step), dir(drv.dir), diag(drv.diag), addr(addr) {}

  // Flag indicating whether the motor is enabled
  // ISR handler skips disabled motors
  volatile bool enabled = false;
  // Temporarily make ISR skip this motor to avoid race conditions
  volatile bool lock = false;

  inline bool isAvailableForISR() { return enabled && !lock; }
  // ISR maintained state
  Micros last_step;
  Steps steps;
  Interval interval;

  // Pending move commands [Producer: main thread | Consumer: ISR]
  RingBuffer<Command, 256> pending;
  // Completed move commands [Producer: ISR | Consumer: main thread]
  RingBuffer<Sequence, 512> done;

  inline bool online() { return driver.test_connection() == 0; }
  void updateConfig(const Protocol::MotorConfig::Config *cfg = nullptr);

  Protocol::MotorConfig::Config config = {
      .micro_steps = 32,
      .stall_sensitivity = 40,
      .rms_current = 1000,
  };

  inline void init() {
    dir.init();
    step.init();
    diag.init();
    driver.begin();
    this->disable();
  }
  inline void enable() {
    if (enabled)
      return;
    updateConfig();
    driver.toff(5);
    last_step = micros();
    enabled = true;
  }
  inline void disable() {
    enabled = false;
    driver.toff(0);
    // Resolve all completed commands
    while (done.readable()) {
      Global::tx.send(done.peek(), Protocol::Method::ACK,
                      Protocol::Property::MOT_MOV);
      done.pop();
    }
    // Reject all pending commands
    while (pending.readable()) {
      Global::tx.print(pending.peek().seq, Protocol::Method::REJ,
                       Protocol::Property::MOT_MOV, "Motor Disabled");
      pending.pop();
    }
    // Reset ISR maintained state
    steps = 0;
    interval = 0;
  }
};

} // namespace Motor

extern Motor::Motor motors[3];

inline Motor::Motor *getMotorByID(MotorID id) {
  for (auto &motor : motors) {
    if (motor.addr == id)
      return &motor;
  }
  return nullptr;
}

namespace Motor {

inline void init() {
  Board::Drv::init();
  Board::Drv::enable();
  for (auto &motor : motors)
    motor.init();
}

} // namespace Motor
