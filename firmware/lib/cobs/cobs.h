// =============================================================================
// This is a C implementation of the Consistent Overhead Byte Stuffing (COBS)
// algorithm.
// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define COBS_MAX_CONTENT 254
#define COBS_MAX_ENCODED (COBS_MAX_CONTENT + 2)

namespace COBS {

static constexpr int16_t UNFINISHED = 0;
static constexpr int16_t ERR_OVERFLOW = -1;
static constexpr int16_t ERR_UNEXPECTED_ZERO = -2;
static constexpr int16_t ERR_UNEXPECTED_END = -3;

class Buffer {
public:
  union {
    uint8_t index = 0;
    uint8_t length;
  };
  uint8_t counter = 0;
  uint8_t data[COBS_MAX_ENCODED];
  void reset() {
    index = 0;
    counter = 0;
  }
};

class RX : public Buffer {
public:
  template <typename T> inline const T &payload() {
    return static_cast<T>(*data);
  }
  int16_t decode(bool (*available)(), char (*read)());
  // For debugging
  uint8_t raw[512];
  unsigned raw_index = 0;
  inline void reset() {
    Buffer::reset();
    raw_index = 0;
  }
};

class TX : public Buffer {
public:
  int16_t encode(const uint8_t *input, const uint8_t len);
  inline uint8_t *payload() { return data; }
  inline uint8_t size() const {
    if (index == 0) {
      return 0; // No data encoded
    } else {
      return index + 2;
    }
  }
};

const char *errorno(int16_t code);

} // namespace COBS
