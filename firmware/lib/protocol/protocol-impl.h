// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
#pragma once
#include <cstdint>

#define __packed__ struct __attribute__((packed))
#define PACKET(NAME, DATA) typedef __packed__ NAME DATA NAME

// Motor Steps
typedef uint8_t MotorID;
typedef int32_t Steps;     // Range -2147483648 to 2147483647
typedef uint32_t Interval; // Range 0-4294967295us (~4294s)

namespace Protocol {

typedef bool SystemEnable;
static_assert(sizeof(SystemEnable) == 1, "SystemEnable must be 1 byte");

PACKET(MotorHeader, { MotorID id; });

typedef enum : uint8_t {
  DIR_FORWARD = 0b01,
  DIR_BACKWARD = 0b10,
} Direction;

PACKET(MotorEnable, {
  MotorID id;
  bool enable; // Enable or disable
});

PACKET(MotorConfig, {
  MotorID id;
  __packed__ Config {
    uint8_t micro_steps;       // 1, 2, 4, 8, 16, 32, 64, 128, 256
    uint8_t stall_sensitivity; // 0-255, 0 = disabled
    uint16_t rms_current;      // mA
  }
  config;
});

PACKET(MotorMove, {
  MotorID id;
  Steps steps;       // Step count
  Interval interval; // Step intervals in us
});

}; // namespace Protocol

#undef PACKET