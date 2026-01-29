// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
#pragma once

#include <cobs.h>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <stdint.h>

namespace Protocol {

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
  MOT_HOME = 0x5,
  MOT_STAT = 0x6,
  LED_PROG = 0xA,
  ODOM_SENSOR = 0xB,
  COLOR_SENSOR = 0xC,
  FW_INFO = 0xF,
} Property;

struct __attribute__((packed)) Header {
  uint8_t checksum;
  uint8_t code; // higher 4 bits = method, lower 4 bits = property
  static inline constexpr uint8_t compose(Method m, Property p) {
    return (static_cast<uint8_t>(m) & 0xF0) | (static_cast<uint8_t>(p) & 0x0F);
  }
  inline void set(Method m, Property p = NA) { code = compose(m, p); }
  inline Method method() const { return static_cast<Method>(code & 0x0F); }
  inline Property property() const {
    return static_cast<Property>(code & 0xF0);
  }
  inline uint8_t compute_checksum(const uint8_t *payload, size_t size) const {
    uint8_t result = code; // Reset checksum to code
    for (size_t i = 0; i < size; i++)
      result ^= payload[i]; // XOR with payload bytes
    return result;
  }
};

static_assert(sizeof(Header) == 2, "Size of Header should be 2 byte");

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
  template <typename T> constexpr inline const T &as() const {
    return *reinterpret_cast<const T *>(&payload);
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
  inline RX(bool (*available)(), char (*read)())
      : available(available), read(read) {
    cobs.reset();
    frame.reset();
  }
  inline void recv() {
    if (valid)
      throw std::runtime_error("Valid frame pending in RX memory");
    auto ret = cobs.decode(available, read);
    if (ret != COBS::UNFINISHED) {
      if (ret >= static_cast<signed>(sizeof(frame.header))) {
        frame.payload_size = ret - sizeof(frame.header);
        memcpy(&frame.buffer, cobs.data, ret);
        valid = frame.validate();
      }
      cobs.reset();
    }
  }
};

class TX {
  size_t (*const write)(const void *buf, size_t size);

public:
  COBS::TX cobs;
  Frame frame;

  inline TX(size_t (*write)(const void *buf, size_t size)) : write(write) {
    cobs.reset();
    frame.reset();
  }

  template <typename T>
  inline void write_frame(Method method, Property property, const T payload) {
    write_frame(method, property, &payload, sizeof(T));
  }

  template <typename T>
  inline void write_frame(Method method, Property property, const T *payload) {
    write_frame(method, property, payload, sizeof(T));
  }

  void write_frame(Method method, Property property, const void *payload,
                   size_t size);
  void encode_frame();

  inline size_t send_frame() { return write(cobs.payload(), cobs.size()); }

  template <typename T>
  inline void send(Method method, Property property, const T &payload) {
    write_frame(method, property, &payload, sizeof(T));
    encode_frame();
    send_frame();
  }

  inline void send(Method method, Property property) {
    frame.header.set(method, property);
    frame.payload_size = 0; // No payload
    encode_frame();
    send_frame();
  }

  template <typename T>
  inline size_t send(Method method, Property property, const T *payload) {
    write_frame(method, property, payload, sizeof(T));
    encode_frame();
    return send_frame();
  }

  inline size_t send(Method method, Property property, void *payload,
                     uint8_t size) {
    write_frame(method, property, payload, size);
    encode_frame();
    return send_frame();
  }
};

} // namespace Protocol
