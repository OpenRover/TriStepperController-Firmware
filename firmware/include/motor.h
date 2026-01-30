// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
#pragma once

#include <cstdint>

#include <TMCStepper.h>

#include "board.h"
#include "esp32-hal.h"

#include <protocol-impl.h>
#include <protocol.h>
#include <scheduler.h>

namespace Motor {

void init();

typedef signed long Steps;

inline int direction(const bool &&forward, const bool &invert) {
  const auto dir = forward ? HIGH : LOW;
  return invert ? !dir : dir;
}

typedef struct State {
  enum : uint8_t {
    MODE_NONE = 0,
    MODE_HOME,
    MODE_NORM,
  } mode;
  Scheduler::Micros step_time;
  Steps position;
  union {
    struct {
      Protocol::Direction direction;
      uint8_t endstop; // 0 for sensor-less homing, others for endstop ID
      enum : uint8_t {
        HOME_INIT, // Untangle endstop switch by moving reverse
        HOME_MOVE, // Move forward until endstop is triggered
        HOME_DONE, // Homing is done, waiting for next command
      } status;
      inline void init(const Protocol::MotorHome &cmd) {
        status = HOME_INIT;
        direction = cmd.direction;
        endstop = cmd.endstop;
      }
    } home;
    struct {
      Steps target; // Target position in steps
    } norm;
  };
  void reset() {
    mode = MODE_NONE;
    step_time = 0;
    position = 0;
  }
} State;

class Motor : public Scheduler::Task {
private:
  void update_config();
  void wait_online();
  void subroutine_home(Scheduler::Micros now);
  void subroutine_norm(Scheduler::Micros now);

public:
  const Board::Pin &step, &dir, &diag;
  const uint8_t addr;
  TMC2209Stepper driver;
  State state;
  Protocol::MotorConfig::Config config = {
      .invert = false,
      .micro_steps = 32,
      .stall_sensitivity = 40,
      .rms_current = 1000,
  };

  class Subtask : public Scheduler::Task {
  public:
    Motor &motor;
    Subtask(Motor &motor)
        : Scheduler::Task(Scheduler::Task::Once{false}), motor(motor) {}
    void tick(Scheduler::Micros now) override;
  } subtask;

  inline Motor(Board::Drv &drv, uint8_t addr)
      : Scheduler::Task(Scheduler::Task::Once{false}),
        driver(TMC2209Stepper(reinterpret_cast<Stream *>(&Board::Drv::serial),
                              0.11f, addr)),
        step(drv.step), dir(drv.dir), diag(drv.diag), addr(addr),
        subtask(*this) {
    state.reset();
  }

  inline bool enabled() { return state.mode != State::MODE_NONE; };
  void init();
  void enable();
  void disable();
  void configure(const Protocol::MotorConfig::Config &cfg);
  void configure(const Protocol::MotorConfig::Config &&cfg);
  void tick(Scheduler::Micros now) override;

  // Blocking function
  inline void run(double revolutions, Scheduler::Micros step_time) {
    if (!enabled())
      throw std::runtime_error("Motor is not enabled");
    const auto steps =
        static_cast<Steps>(std::abs(revolutions) * 20 * config.micro_steps);
    if (steps <= 0)
      return;
    const auto d = direction(revolutions > 0.0, config.invert);
    if (dir.read() != d) {
      dir.write(d);
      delayMicroseconds(1);
    }
    for (unsigned long i = 0; i < steps; i++) {
      step.write(HIGH);
      delayMicroseconds(1);
      step.write(LOW);
      delayMicroseconds(step_time > 2_us ? step_time - 1_us : 1_us);
    }
  }
};

} // namespace Motor

extern Motor::Motor motors[3];
