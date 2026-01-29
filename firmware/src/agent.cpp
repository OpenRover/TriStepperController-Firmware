// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
#include "agent.h"
#include "motor.h"
#include "protocol.h"
#include "utils.h"
#include "version.h"

#include <protocol-impl.h>

Agent agent(Global::rx, Global::tx);

using namespace Scheduler;
using namespace Protocol;

static const char identity[] = IDENTITY;

#define HEADER(METHOD, PROPERTY)                                               \
  Header::compose(Method::METHOD, Property::PROPERTY)

#define UNSUPPORTED(METHOD, PROPERTY)                                          \
  case HEADER(METHOD, PROPERTY):                                               \
    tx.send(Method::REJ, Property::PROPERTY);                                  \
    WARN("%s is not supported", #METHOD "::" #PROPERTY);                       \
    break;

#define CHECKED(type, prop, code)                                              \
  if (!frame.check<type>()) {                                                  \
    tx.send(Method::REJ, Property::prop, "Payload size mismatch");             \
  } else {                                                                     \
    auto &cmd = frame.as<Protocol::MotorHeader>();                             \
    code;                                                                      \
  }

void Agent::tick(Micros) {
  if (!rx.valid)
    rx.recv();
  if (!rx.valid)
    return;
  rx.valid = false;
  const auto &frame = rx.frame;
  const auto &code = frame.header.code;
  switch (code) {
  case HEADER(GET, FW_INFO):
    tx.write_frame(Method::ACK, Property::FW_INFO, identity, sizeof(identity));
    tx.encode_frame();
    tx.send_frame();
    break;
    UNSUPPORTED(SET, FW_INFO);
  case HEADER(SET, SYS_ENA): {
    const auto &enable = frame.as<Protocol::SystemEnable>();
    if (enable) {
      Board::Drv::enable();
    } else {
      for (auto &motor : motors)
        motor.disable();
      Board::Drv::disable();
    }
    tx.send(Method::ACK, Property::SYS_ENA, Board::Drv::is_enabled());
    break;
  }
  case HEADER(GET, SYS_ENA):
    tx.send(Method::ACK, Property::SYS_ENA, Board::Drv::is_enabled());
    break;
  case HEADER(GET, MOT_ENA): {
    auto &cmd = frame.as<Protocol::MotorHeader>();
    for (auto &motor : motors) {
      if (motor.addr != cmd.id)
        continue;
      const Protocol::MotorEnable res = {
          .id = cmd.id,
          .enable = motor.enabled(),
      };
      tx.send(Method::ACK, Property::MOT_ENA, res);
    }
  }
  case HEADER(SET, MOT_ENA): {
    auto &cmd = frame.as<Protocol::MotorEnable>();
    for (auto &motor : motors) {
      if (motor.addr != cmd.id)
        continue;
      if (cmd.enable)
        motor.enable();
      else
        motor.disable();
      tx.send(Method::ACK, Property::MOT_ENA, cmd);
    }
    break;
  }
  case HEADER(GET, MOT_CFG): {
    auto &cfg = frame.as<Protocol::MotorHeader>();
    for (auto &motor : motors) {
      if (motor.addr != cfg.id)
        continue;
      Protocol::MotorConfig cfg;
      cfg.id = motor.addr;
      cfg.config = motor.config;
      tx.send(Method::ACK, Property::MOT_CFG, cfg);
    }
    break;
  }
  case HEADER(SET, MOT_CFG): {
    auto &cfg = frame.as<Protocol::MotorConfig>();
    for (auto &motor : motors) {
      if (motor.addr != cfg.id)
        continue;
      motor.configure(cfg.config);
      tx.send(Method::ACK, Property::MOT_CFG, cfg);
    }
    break;
  }
  case HEADER(GET, MOT_MOV): {
    const auto &cmd = frame.as<Protocol::MotorHeader>();
    for (auto &motor : motors) {
      if (motor.addr != cmd.id)
        continue;
      const Protocol::MotorMove res = {
          .id = cmd.id,
          .target = motor.state.norm.target,
          .step_time = motor.state.step_time,
      };
      tx.send(Method::ACK, Property::MOT_MOV, res);
    }
    break;
  }
  case HEADER(SET, MOT_MOV): {
    const auto &cmd = frame.as<Protocol::MotorMove>();
    for (auto &motor : motors) {
      if (motor.addr != cmd.id)
        continue;
      if (cmd.step_time == 0) {
        motor.state.position = cmd.target;
        const Protocol::MotorMove res = {
            .id = cmd.id,
            .target = cmd.target,
            .step_time = cmd.step_time,
        };
        tx.send(Method::ACK, Property::MOT_MOV, res);
        continue;
      }
      if (!motor.enabled()) {
        tx.send(Method::REJ, MOT_MOV, cmd);
        WARN("MOT_MOV Rejected: %s", "Motor not enabled");
        continue;
      }
      motor.state.mode = Motor::State::MODE_NORM;
      motor.state.step_time = cmd.step_time;
      motor.state.norm.target = cmd.target;
      if (!motor.task_once.pending)
        motor.schedule(micros());
    }
    break;
  }
    UNSUPPORTED(GET, MOT_HOME);
  case HEADER(SET, MOT_HOME): {
    const auto &cmd = frame.as<Protocol::MotorHome>();
    for (auto &motor : motors) {
      if (motor.addr != cmd.id)
        continue;
      if (!motor.enabled()) {
        tx.send(Method::REJ, MOT_HOME, cmd);
        WARN("MOT_HOME Rejected: %s", "Motor not enabled");
        continue;
      }
      motor.state.mode = Motor::State::MODE_HOME;
      motor.state.home.init(cmd);
      if (!motor.task_once.pending)
        motor.schedule(micros());
    }
    break;
  }
    UNSUPPORTED(SET, MOT_STAT);
  case HEADER(GET, MOT_STAT): {
    const auto &cmd = frame.as<Protocol::MotorHeader>();
    for (auto &motor : motors) {
      if (motor.addr != cmd.id)
        continue;
      const Protocol::MotorStatus res = {
          .id = cmd.id,
          .diag_pin = motor.diag.read(),
          .sg_result = motor.driver.SG_RESULT(),
          .position = motor.state.position,
      };
      tx.send(Method::ACK, Property::MOT_STAT, res);
    }
    break;
  }
  default:
    WARN("Frame ignored (header = 0x%02X)", code);
    break;
  }
}
