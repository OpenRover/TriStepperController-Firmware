// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================

#include "protocol.h"
#include "debug.h"

namespace Protocol {

RX::RX(bool (*available)(), char (*read)()) : available(available), read(read) {
  cobs.reset();
  frame.reset();
}

void RX::recv() {
  if (valid)
    return;
  const auto ret = cobs.decode(available, read);
  if (ret == COBS::UNFINISHED)
    return;
  if (ret > 0) {
    if (ret >= static_cast<int>(sizeof(frame.header))) {
      frame.payload_size = ret - sizeof(frame.header);
      memcpy(&frame.buffer, cobs.data, ret);
      valid = frame.validate();
      if (!valid) {
        DEBUG("❌ RX Packet CRC check failed\n  Raw [");
        for (uint8_t i = 0; i < cobs.raw_index; i++) {
          DEBUG(" %02X", cobs.raw[i]);
        }
        DEBUG(" ]\n  Dec [");
        for (uint8_t i = 0; i < ret; i++) {
          DEBUG(" %02X", ((uint8_t *)&frame.buffer)[i]);
        }
        DEBUG(" ]\n");
      }
    } else {
      DEBUG("📦 RX Packet too short: %d bytes\n", ret);
    }
  } else {
    DEBUG("⚠️ RX COBS decode error %d: %s\n  Raw [", ret, COBS::errorno(ret));
    for (uint8_t i = 0; i < cobs.raw_index; i++) {
      DEBUG(" %02X", cobs.raw[i]);
    }
    DEBUG(" ]\n");
  }
  cobs.reset();
}

void RX::reset() {
  valid = false;
  cobs.reset();
  frame.reset();
}

TX::TX(size_t (*write)(const void *buf, size_t size)) : write(write) {
  cobs.reset();
  frame.reset();
}

int TX::write_frame(Sequence s, Method m, Property p, const void *payload,
                    size_t size) {
  if (size > sizeof(frame.payload))
    size = sizeof(frame.payload);
  frame.header.set(s, m, p);
  memcpy(&frame.payload, payload, size);
  frame.payload_size = size;
  return size;
}

int TX::encode_frame() {
  frame.checksum();
  return cobs.encode((uint8_t *)&frame.buffer, frame.size());
}

} // namespace Protocol
