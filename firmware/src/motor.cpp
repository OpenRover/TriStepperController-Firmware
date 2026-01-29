// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
#include "TMCStepper.h"
#include "board.h"
#include "protocol-impl.h"
#include "utils.h"
#include <motor.h>

Motor::Motor motors[3] = {
    {Board::DRV[0], 0}, {Board::DRV[1], 1}, {Board::DRV[2], 2}};

namespace Motor {

void Motor::update_config() {
  // Blank time controls step timing, 0 = no blanking, 1-3 = 1us, 4-7 = 2us,
  // etc.
  driver.blank_time(2);

  driver.rms_current(config.rms_current);
  driver.microsteps(config.micro_steps);
  // driver.en_pwm_mode(true); // Toggle stealthChop on TMC2130/2160/5130/5160
  driver.en_spreadCycle(false); // Toggle spreadCycle on TMC2208/2209/2224
  driver.pwm_autoscale(true);   // Needed for stealthChop
  driver.semin(5);
  driver.semax(2);
  driver.sedn(0b01);
  driver.IOIN(); // Disable all IO pins
  // DIAG is pulsed by StallGuard when SG_RESULT falls below SGTHRS. It is
  // only enabled in StealthChop mode, and when TCOOLTHRS ≥ TSTEP > TPWMTHRS
  driver.TCOOLTHRS(0xFFFFF); // 20bit max
  driver.TPWMTHRS(0x00000);  // 20bit max
  driver.SGTHRS(config.stall_sensitivity);
}

void Motor::wait_online() {
  while (driver.test_connection() != 0) {
    LOG("Waiting for TMC Driver %u", addr);
  };
  LOG("TMC Driver %u Online", addr);
}

void init() {
  Board::Drv::serial.setPins(RX, TX);
  Board::Drv::serial.begin(Board::Drv::baud);
  pinMode(Board::Drv::EN, OUTPUT);
  Board::Drv::enable();
  for (auto &motor : motors)
    motor.init();
}

void Motor::init() {
  dir.init();
  step.init();
  diag.init();
  driver.begin();
  Motor::disable();
}

void Motor::enable() {
  state.reset();
  wait_online();
  driver.toff(4); // Enables driver in software, changed from 5
  update_config();
  state.mode = State::MODE_NORM;
}

void Motor::disable() {
  state.reset();
  driver.toff(0); // Disables driver in software
  state.mode = State::MODE_NONE;
}

void Motor::configure(const Protocol::MotorConfig::Config &cfg) {
  config = cfg;
  if (enabled())
    update_config();
}

void Motor::configure(const Protocol::MotorConfig::Config &&cfg) {
  config = cfg;
  if (enabled())
    update_config();
}

void Motor::tick(Scheduler::Micros now) {
  switch (state.mode) {
  case State::MODE_HOME:
    return subroutine_home(now);
  case State::MODE_NORM:
    return subroutine_norm(now);
  default:
    return;
  }
};

void Motor::Subtask::tick(Scheduler::Micros now) {
  motor.step.write(LOW);
  if (motor.dir.read() ^ motor.config.invert) {
    motor.state.position += 1;
  } else {
    motor.state.position -= 1;
  }
}

void Motor::subroutine_home(Scheduler::Micros now) {}

void Motor::subroutine_norm(Scheduler::Micros now) {
  const auto &position = state.position, target = state.norm.target;
  // If motor has reached the target position, send ACK and terminate the task.
  if (position == target) {
    const Protocol::MotorMove res = {
        .id = addr,
        .target = target,
        .step_time = state.step_time,
    };
    Global::tx.send(Protocol::Method::ACK, Protocol::Property::MOT_MOV, res);
    return;
  }
  // Execute one step.
  const auto dir = direction(target > position, config.invert);
  if (this->dir.read() != dir) {
    this->dir.write(dir);
    return this->sleep(now, 1_us);
  }
  this->step.write(HIGH);
  this->subtask.schedule(now + 1_us);
  return this->sleep(now, state.step_time >= 2_us ? state.step_time : 2_us);
}

} // namespace Motor
