// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
#pragma once

#include <cobs.h>
#include <cstdint>
#include <cstring>
#include <stdint.h>

#include "convert.h"

namespace Protocol {

typedef uint16_t Sequence;

typedef enum Method : uint8_t {
  NOP = 0x00,
  // HOST -> DEVICE
  GET = 0x10,
  SET = 0x20,
  // DEVICE -> HOST
  ACK = 0x30,
  REJ = 0x40,
  // DEVICE -> HOST, for asynchronous events
  SYN = 0x80,
  // Special Log Method
  LOG = 0xF0,
} Method;

typedef enum Property : uint8_t {
  NA = 0x0,
  SYS_ENA = 0x1,
  MOT_ENA = 0x2,
  MOT_CFG = 0x3,
  MOT_MOV = 0x4,
  BARRIER = 0xE, // Reserved for multi-axis synchronization
  FW_INFO = 0xF,
} Property;

} // namespace Protocol

#define CASE(K)                                                                \
  case Protocol::Method::K:                                                    \
    return #K;
template <> inline const char *const convert(const Protocol::Method &value) {
  switch (value) {
    CASE(NOP);
    CASE(GET);
    CASE(SET);
    CASE(ACK);
    CASE(REJ);
    CASE(SYN);
    CASE(LOG);
  default:
    return "UNKNOWN_METHOD";
  }
}
#undef CASE

#define CASE(P)                                                                \
  case Protocol::Property::P:                                                  \
    return #P;
template <> inline const char *const convert(const Protocol::Property &value) {
  switch (value) {
    CASE(NA);
    CASE(SYS_ENA);
    CASE(MOT_ENA);
    CASE(MOT_CFG);
    CASE(MOT_MOV);
    CASE(BARRIER);
    CASE(FW_INFO);
  default:
    return "UNKNOWN_PROPERTY";
  }
}
#undef CASE
