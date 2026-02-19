// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
#include "agent.h"
#include "board.h"
#include "debug.h"
#include "esp32-hal.h"
#include "esp_task_wdt.h"
#include "global.h"
#include "motor.h"
#include "protocol-impl.h"
#include "protocol.h"
#include "version.h"

using namespace Protocol;
using Global::rx;
using Global::tx;

static const char identity[] = IDENTITY;

#define HEADER(METHOD, PROPERTY)                                               \
  Header::compose(Method::METHOD, Property::PROPERTY)

#define REPLY(M, P, ...)                                                       \
  TRACE("tx.send(" #M "::" #P ")");                                            \
  tx.send(seq, Method::M, Property::P __VA_OPT__(, ) __VA_ARGS__)

#define PRINT(M, P, MSG)                                                       \
  TRACE("tx.print(" #M "::" #P ")");                                           \
  tx.print(seq, Method::M, Property::P, MSG)

static constexpr auto BAD_PAYLOAD = "Invalid payload";
static constexpr auto NO_SUCH_MOTOR = "No such motor";
static constexpr auto MOTOR_OFFLINE = "Motor Offline";
static constexpr auto MOTOR_DISABLED = "Motor Disabled";
static constexpr auto MOTOR_QUEUE_FULL = "Motor Queue Full";

#define HANDLE_COMMAND(METHOD, PROP, PAYLOAD_TYPE, CODE)                       \
  case HEADER(METHOD, PROP): {                                                 \
    TRACE(#METHOD "::" #PROP);                                                 \
    const auto cmd = frame.as<PAYLOAD_TYPE>();                                 \
    if (cmd == nullptr) {                                                      \
      PRINT(REJ, PROP, BAD_PAYLOAD);                                           \
      break;                                                                   \
    }                                                                          \
    CODE;                                                                      \
    break;                                                                     \
  }

#define MOTOR_COMMAND(PROP, CODE)                                              \
  auto motor = getMotorByID(cmd->id);                                          \
  if (motor) {                                                                 \
    CODE                                                                       \
  } else {                                                                     \
    PRINT(REJ, PROP, NO_SUCH_MOTOR);                                           \
  }

inline void processFrame(const Frame &frame) {
  const auto &seq = frame.header.sequence;
  const auto &code = frame.header.code;
  DEBUG("RX [%06d] %s::%s\n", seq,
        convert<const char *const>(frame.header.method()),
        convert<const char *const>(frame.header.property()));
  switch (code) {
  case HEADER(GET, FW_INFO):
    TRACE("GET::FW_INFO");
    PRINT(ACK, FW_INFO, identity);
    break;
  case HEADER(SET, SYS_ENA): {
    TRACE("SET::SYS_ENA");
    const auto enable = frame.as<Protocol::SystemEnable>();
    if (enable == nullptr) {
      PRINT(REJ, SYS_ENA, BAD_PAYLOAD);
      break;
    } else if (*enable) {
      Board::Drv::enable();
    } else {
      for (auto &motor : motors)
        // Not checking motor online here because master EN command should
        // always take effect
        motor.disable();
      Board::Drv::disable();
    }
    REPLY(ACK, SYS_ENA, Board::Drv::is_enabled());
    break;
  }
  case HEADER(GET, SYS_ENA):
    TRACE("GET::SYS_ENA");
    REPLY(ACK, SYS_ENA, Board::Drv::is_enabled());
    break;
    HANDLE_COMMAND(GET, MOT_ENA, Protocol::MotorHeader, {
      MOTOR_COMMAND(MOT_ENA, {
        REPLY(ACK, MOT_ENA,
              Protocol::MotorEnable{
                  .id = cmd->id,
                  .enable = motor->enabled,
              });
      });
    });
    HANDLE_COMMAND(SET, MOT_ENA, Protocol::MotorEnable, {
      MOTOR_COMMAND(MOT_ENA, {
        if (cmd->enable != motor->enabled) {
          if (!motor->online()) {
            PRINT(REJ, MOT_ENA, MOTOR_OFFLINE);
            break;
          }
          if (cmd->enable)
            motor->enable();
          else
            motor->disable();
        };
        REPLY(ACK, MOT_ENA,
              Protocol::MotorEnable{
                  .id = cmd->id,
                  .enable = motor->enabled,
              });
      });
    });
    HANDLE_COMMAND(GET, MOT_CFG, Protocol::MotorHeader, {
      MOTOR_COMMAND(MOT_CFG, {
        REPLY(ACK, MOT_CFG,
              Protocol::MotorConfig{
                  .id = motor->addr,
                  .config = motor->config,
              });
      });
    });
    HANDLE_COMMAND(SET, MOT_CFG, Protocol::MotorConfig, {
      MOTOR_COMMAND(MOT_CFG, {
        if (motor->online()) {
          motor->updateConfig(&cmd->config);
          REPLY(ACK, MOT_CFG,
                Protocol::MotorConfig{
                    .id = motor->addr,
                    .config = motor->config,
                });
        } else {
          PRINT(REJ, MOT_CFG, MOTOR_OFFLINE);
        }
      });
    });
    HANDLE_COMMAND(SET, MOT_MOV, Protocol::MotorMove, {
      MOTOR_COMMAND(MOT_MOV, {
        if (!motor->enabled) {
          PRINT(REJ, MOT_MOV, MOTOR_DISABLED);
          break;
        }
        if (!motor->pending.writable()) {
          PRINT(REJ, MOT_MOV, MOTOR_QUEUE_FULL);
          break;
        }
        // DEBUG("Motor%d %d steps @ %dus\n", motor->addr, cmd->steps,
        // cmd->interval);
        motor->pending.push(Motor::Command{seq, cmd->steps, cmd->interval});
        // Delay ACK until the pending move is applied by ISR handler.
      });
    });
  default: {
    static char buffer[254];
    auto len = snprintf(buffer, sizeof(buffer), "Unsupported command: %s::%s",
                        convert<const char *const>(frame.header.method()),
                        convert<const char *const>(frame.header.property()));
    if (len > 0)
      tx.send(seq, Method::REJ, Property::NA, buffer, len);
    break;
  }
  }
}

inline void checkSerial() {
  static bool connected = false;
  if (!Serial) {
    if (connected) {
      connected = false;
      DEBUG("Host disconnected\n");
    }
  } else {
    if (!connected) {
      connected = true;
      DEBUG("Host connected\n");
    }
  }
}

extern uint32_t isr_active_cycles, isr_yield_cycles, isr_cycle_count;

void agent(void *) {
  while (true) {
    TRACE("agentTick()");
    agentTick();
    TRACE_EXIT();
    delay(1);
  }
}

#define MOTOR_ACK(M)                                                           \
  TRACE(#M ".done.readable()");                                                \
  while (M.done.readable()) {                                                  \
    TRACE(#M ".done.peek()");                                                  \
    const auto &seq = M.done.peek();                                           \
    REPLY(ACK, MOT_MOV);                                                       \
    TRACE(#M ".done.peek()");                                                  \
    M.done.pop();                                                              \
    TRACE(#M ".done.readable()");                                              \
  }                                                                            \
  TRACE(#M " [TX COMPLETE]");

int serialAvailable() {
  TRACE("serialAvailable() query");
  int available = Serial.available();
  TRACE("serialAvailable() done");
  return available;
}

void agentTick() {
  // Feed hardware watchdog to prevent reset
  esp_task_wdt_reset();
  static unsigned long last_report = millis();
  static constexpr unsigned REPORT_INTERVAL = 10000;
  if (millis() - last_report >= REPORT_INTERVAL) {
    last_report = millis();
    const auto volatile active = isr_active_cycles;
    const auto volatile yield = isr_yield_cycles;
    const auto volatile count = isr_cycle_count;
    isr_cycle_count = 0;
    DEBUG(""
          "ISR %.2fKHz"
          ", "
          "Active %u cycles"
          ", "
          "Yield %u cycles (%.2f%%)"
          "\n",
          count / (float)REPORT_INTERVAL, active, yield,
          100.0 * active / (active + yield));
    for (auto &motor : motors) {
      if (motor.enabled) {
        DEBUG("Motor %d [%d steps @ %u us] Pending=%u\n", motor.addr,
              motor.steps, motor.interval, motor.pending.len());
      }
    }
  }
  TRACE("checkSerial()");
  checkSerial();
  if (!Serial) {
    if (Board::Drv::is_enabled()) {
      Board::Drv::disable();
      for (auto &motor : motors)
        motor.disable();
    }
    return;
  }
  TRACE("Motor ACK TX");
  MOTOR_ACK(motors[0]);
  MOTOR_ACK(motors[1]);
  MOTOR_ACK(motors[2]);
  while (serialAvailable() > 0) {
    TRACE("Process RX");
    if (!rx.valid) {
      TRACE("rx.recv()");
      rx.recv();
    }
    TRACE("Check RX valid");
    if (!rx.valid)
      continue;
    TRACE("processFrame()");
    processFrame(rx.frame);
    TRACE("Reset RX valid");
    rx.valid = false;
  }
}
