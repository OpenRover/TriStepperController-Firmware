// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
#include "board.h"
#include <motor.h>
#include <numeric>

#include "debug.h"

uint32_t isr_active_cycles = 0, isr_yield_cycles = 0, isr_cycle_count = 0;
Motor::Motor motors[3] = {
    {Board::DRV[0], 0}, {Board::DRV[1], 1}, {Board::DRV[2], 2}};

#define TRACE_MOTOR(...)                                                       \
  do {                                                                         \
    switch (motor.addr) {                                                      \
    case 0:                                                                    \
      TRACE("[M0] " __VA_ARGS__);                                              \
      break;                                                                   \
    case 1:                                                                    \
      TRACE("[M1] " __VA_ARGS__);                                              \
      break;                                                                   \
    case 2:                                                                    \
      TRACE("[M2] " __VA_ARGS__);                                              \
      break;                                                                   \
    }                                                                          \
  } while (0)

void IRAM_ATTR motorTick() {
  TRACE("motorTick()");
  static uint32_t tp0, tp1;
  tp0 = ESP.getCycleCount();
  isr_yield_cycles = tp0 - tp1;
  const Micros now = micros();
  for (auto &motor : motors) {
    // Skip motor if disabled or locked
    // TRACE_MOTOR("isAvailableForISR()");
    if (!motor.isAvailableForISR())
      continue;
    // Execute pending motion
    const auto elapsed = now - motor.last_step;
    if (elapsed < motor.interval)
      continue;
    motor.last_step = now;
    // Generate step pulse if necessary
    if (motor.steps > 0) {
      // TRACE_MOTOR("step.toggle()");
      motor.step.toggle();
      motor.steps--;
    }
    if (motor.steps < 0) {
      // TRACE_MOTOR("step.toggle()");
      motor.step.toggle();
      motor.steps++;
    }
    if (motor.steps != 0)
      continue;
    // Obtain next command, if available
    // TRACE_MOTOR("pending.readable()");
    if (!motor.pending.readable())
      continue;
    // TRACE_MOTOR("pending.peek()");
    auto &cmd = motor.pending.peek();
    // TRACE_MOTOR("done.writable()");
    if (motor.done.writable()) {
      // TRACE_MOTOR("done.push()");
      motor.done.push(cmd.seq);
    }
    motor.steps = cmd.steps;
    motor.interval = cmd.interval;
    // TRACE_MOTOR("pending.pop()");
    motor.pending.pop();
    // Flip direction pin if needed, leave enough step hold time
    // TRACE_MOTOR("setting direction");
    if (motor.steps > 0 && motor.dir.read() != HIGH)
      motor.dir.write(HIGH);
    if (motor.steps < 0 && motor.dir.read() != LOW)
      motor.dir.write(LOW);
  }
  TRACE("motorTick() complete");
  tp1 = ESP.getCycleCount();
  isr_active_cycles = tp1 - tp0;
  isr_cycle_count++;
  TRACE_EXIT();
};

void Motor::Motor::updateConfig(const Protocol::MotorConfig::Config *cfg) {
  if (cfg)
    config = *cfg;
  // Blank time controls step timing, 0 = no blanking, 1-3 = 1us, 4-7 = 2us,
  // etc.
  driver.blank_time(2);
  driver.rms_current(config.rms_current);
  driver.microsteps(config.micro_steps);
  driver.en_spreadCycle(false); // Toggle spreadCycle on TMC2208/2209/2224
  driver.pwm_autoscale(true);   // Needed for stealthChop
  driver.semin(5);
  driver.semax(2);
  driver.sedn(0b01);
  driver.dedge(true); // Use both edges of STEP for stepping
  // DIAG is pulsed by StallGuard when SG_RESULT falls below SGTHRS. It is
  // only enabled in StealthChop mode, and when TCOOLTHRS ≥ TSTEP > TPWMTHRS
  driver.TCOOLTHRS(0xFFFFF); // 20bit max
  driver.TPWMTHRS(0x00000);  // 20bit max
  driver.SGTHRS(config.stall_sensitivity);
  driver.IOIN(); // Disable all IO pins
};
