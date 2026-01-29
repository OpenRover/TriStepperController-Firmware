// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
#pragma once
#include <cstdint>

#define PACKET typedef struct __attribute__((packed))
#define __packed__ struct __attribute__((packed))
namespace Protocol {

typedef bool SystemEnable;

static_assert(sizeof(SystemEnable) == 1,
              "SystemEnable payload should be 1 byte");

typedef uint8_t MotorID;

PACKET MotorHeader { MotorID id; }
MotorHeader;

typedef enum : uint8_t {
  DIR_FORWARD = 0b01,
  DIR_BACKWARD = 0b10,
} Direction;

PACKET MotorEnable {
  MotorID id;
  bool enable; // Enable or disable
}
MotorEnable;

PACKET MotorConfig {
  MotorID id;
  __packed__ Config {
    uint8_t invert;            // forward = HIGH (false) or LOW (true)
    uint8_t micro_steps;       // 1, 2, 4, 8, 16, 32, 64, 128, 256
    uint8_t stall_sensitivity; // 0-255, 0 = disabled
    uint16_t rms_current;      // mA
  }
  config;
}
MotorConfig;

PACKET MotorHome {
  MotorID id;
  // Endstop Callout:
  // 0 - Sensor-less homing (TMCxxxx)
  // others: id of the endstop switch
  uint8_t endstop;
  Direction direction;
  uint64_t step_time; // Step intervals in us
}
MotorHome;

PACKET MotorMove {
  MotorID id;
  int64_t target;     // Target position in steps
  uint64_t step_time; // Step intervals in us
}
MotorMove;

PACKET MotorPosition {
  MotorID id;
  int64_t position; // Current position in steps
}
MotorPosition;

PACKET MotorStatus {
  MotorID id;
  uint8_t diag_pin;   // Diagnostic pin state (true = HIGH, false = LOW)
  uint16_t sg_result; // Stall guard result (0-255)
  int64_t position;   // Current position in steps
}
MotorStatus;

PACKET LED_Display {
  __packed__ Pixel { uint8_t r, g, b; }
  px[36];
}
LED_Display;

}; // namespace Protocol
