// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================

#include "protocol.h"

#include <string>

namespace Protocol {

void TX::write_frame(Method method, Property property, const void *payload,
                     size_t size) {
  if (size > sizeof(frame.payload))
    throw std::runtime_error("Payload size exceeds maximum limit");
  frame.header.set(method, property);
  memcpy(&frame.payload, payload, size);
  frame.payload_size = size;
}

void TX::encode_frame() {
  frame.checksum();
  auto ret = cobs.encode((uint8_t *)&frame.buffer, frame.size());
  if (ret < 0)
    throw std::runtime_error("COBS encoding failed, code = " +
                             std::to_string(ret));
}

} // namespace Protocol
