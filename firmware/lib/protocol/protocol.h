// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
#pragma once

#include <cobs.h>
#include <cstdint>
#include <cstring>
#include <stdint.h>

#include "protocol-header.h"

namespace Protocol {

struct __attribute__((packed)) Header {
  uint8_t checksum;
  union {
    struct {
      uint8_t seq[2];
    };
    uint16_t sequence;
  };
  uint8_t code; // higher 4 bits = m, lower 4 bits = p
  static inline constexpr uint8_t compose(Method m, Property p) {
    return (static_cast<uint8_t>(m) & 0xF0) | (static_cast<uint8_t>(p) & 0x0F);
  }
  inline void set(Sequence s, Method m, Property p = NA) {
    sequence = s;
    code = compose(m, p);
  }
  inline Method method() const { return static_cast<Method>(code & 0xF0); }
  inline Property property() const {
    return static_cast<Property>(code & 0x0F);
  }
  inline uint8_t compute_checksum(const uint8_t *payload, size_t size) const {
    uint8_t result = seq[0] ^ seq[1] ^ code; // Reset checksum to code
    for (size_t i = 0; i < size; i++)
      result ^= payload[i]; // XOR with payload bytes
    return result;
  }
};

static_assert(sizeof(Header) == 4, "Size of Header should be 4 byte");

class Frame {
public:
  static constexpr size_t PAYLOAD_SIZE = COBS_MAX_CONTENT - sizeof(Header);
  union {
    struct __attribute__((packed)) {
      Header header;                 // Header
      uint8_t payload[PAYLOAD_SIZE]; // Payload
    };
    uint8_t buffer[COBS_MAX_CONTENT] = {0}; // Entire buffer
  };
  uint8_t payload_size = 0;
  inline uint8_t size() const { return payload_size + sizeof(header); }
  inline void reset() { payload_size = 0; }
  inline void checksum() {
    header.checksum = header.compute_checksum(payload, payload_size);
  }
  inline bool validate() const {
    return header.checksum == header.compute_checksum(payload, payload_size);
  }
  template <typename T>
  constexpr inline const bool check(size_t size = sizeof(T)) const {
    return size == payload_size;
  }
  template <typename T> inline const T *as() const {
    if (payload_size < sizeof(T))
      return nullptr;
    else
      return reinterpret_cast<const T *>(&payload);
  }
};

static_assert(sizeof(Frame) == COBS_MAX_CONTENT + sizeof(Frame::payload_size),
              "Size of Frame should equal COBS_MAX_CONTENT");

class RX {
  bool (*const available)();
  char (*const read)();

public:
  COBS::RX cobs;
  Frame frame;
  bool valid = false;
  RX(bool (*available)(), char (*read)());
  void recv();
  void reset();
};

#define ARGS Sequence s, Method m, Property p

class TX {
public:
  size_t (*const write)(const void *buf, size_t size);
  COBS::TX cobs;
  Frame frame;
  TX(size_t (*write)(const void *buf, size_t size));

  template <typename T> inline void write_frame(ARGS, const T payload) {
    write_frame(s, m, p, &payload, sizeof(T));
  }

  template <typename T> inline void write_frame(ARGS, const T *payload) {
    write_frame(s, m, p, payload, sizeof(T));
  }

  int write_frame(ARGS, const void *payload, size_t size);
  int encode_frame();

  inline size_t send_frame() { return write(cobs.payload(), cobs.size()); }

  inline size_t send(ARGS) {
    frame.header.set(s, m, p);
    frame.payload_size = 0; // No payload
    encode_frame();
    return send_frame();
  }

  template <typename T> inline size_t send(ARGS, const T &payload) {
    write_frame(s, m, p, &payload, sizeof(T));
    encode_frame();
    return send_frame();
  }

  template <typename T>
  inline size_t send(ARGS, const T *payload, uint8_t count) {
    write_frame(s, m, p, payload, sizeof(T) * count);
    encode_frame();
    return send_frame();
  }

  inline size_t print(ARGS, const char *payload) {
    return send(s, m, p, payload, strlen(payload));
  }
};

#undef ARGS

} // namespace Protocol
