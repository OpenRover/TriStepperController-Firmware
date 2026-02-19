// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
// Serial Transport Control Protocol (STCP)
// =============================================================================
#pragma once

#include <cstdint>

#include <cobs.h>

namespace STCP {

typedef enum : uint8_t {
  STCP_REQ = 0x00,
  STCP_ACK = 0x01,
  STCP_REJ = 0x02,
} PacketType;

typedef struct __attribute__((packed)) Header {
  uint8_t crc;
  uint8_t sequence;
  PacketType type;
  uint8_t size;
  uint8_t payload[0];
} Header;

typedef union Frame {
  Header header;
  uint8_t buffer[256];
} Frame;

constexpr uint8_t MAX_PAYLOAD = sizeof(Frame) - sizeof(Header);

static_assert(sizeof(Header) == 4, "Size of STCP header should be 4 bytes");

typedef int (*Reader)(char *buf, unsigned count);
typedef int (*Writer)(const char *buf, unsigned count);

template <unsigned SIZE = 128> class Port {
public:
  const Reader read;
  const Writer write;
  // Pending outbound frame
  bool out_pending = false;
  unsigned long last_retry = 0; // milliseconds
  unsigned retry_count = 0;
  Frame out_frame;
};

} // namespace STCP
