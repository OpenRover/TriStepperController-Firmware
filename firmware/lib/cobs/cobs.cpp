// =============================================================================
// This is a C implementation of the Consistent Overhead Byte Stuffing (COBS)
// algorithm.
// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
#include "cobs.h"

namespace COBS {

int16_t RX::decode(bool (*available)(), char (*read)()) {
  while (available()) {
    // Handle next byte
    const auto byte = read();
    raw[raw_index++] = byte;
    // Check for reserved zero byte
    if (byte == 0) {
      if (counter == 0)
        continue; // Ignore extra zero bytes
      else if (counter == 1)
        return length; // End of frame
      else
        return ERR_UNEXPECTED_ZERO;
    }
    if (index == 0 && counter == 0) {
      counter = byte;
      continue;
    }
    if (counter == 0)
      return ERR_UNEXPECTED_END;
    if (index >= COBS_MAX_CONTENT)
      return ERR_OVERFLOW;
    // Normal control flow
    if (counter == 1) {
      counter = byte;
      // Treat it as a valid zero data byte
      data[index] = 0;
    } else {
      // Handle non-zero byte
      data[index] = byte;
      counter--;
    }
    index++;
  }
  if (index == COBS_MAX_CONTENT + 1 && counter == 1)
    // Max content length (254 bytes) may omit tailing zero byte
    return COBS_MAX_CONTENT;
  else
    return UNFINISHED;
}

int16_t TX::encode(const uint8_t *input, const uint8_t len) {
  reset();
  for (uint8_t i = 0; i < len; i++) {
    // Check for buffer overflow
    if (index == COBS_MAX_CONTENT) {
      return ERR_OVERFLOW;
    }
    // Forward Iteration
    index++;
    counter++;
    // Handle next byte
    if (input[i] == 0) {
      // Handle zero byte
      data[index - counter] = counter;
      counter = 0;
    } else {
      // Handle non-zero byte
      data[index] = input[i];
    }
  }
  // Temporary fill the counter byte
  data[index - counter] = counter + 1;
  // Append zero byte
  data[index + 1] = 0;
  // Return length including tailing zero byte
  return index + 1;
};

const char *errorno(int16_t code) {
  switch (code) {
  case UNFINISHED:
    return "[[PENDING]]";
  case ERR_OVERFLOW:
    return "Buffer overflow";
  case ERR_UNEXPECTED_ZERO:
    return "Unexpected zero byte";
  case ERR_UNEXPECTED_END:
    return "Unexpected end of data";
  default:
    if (code < 0)
      return "Unknown error";
    else
      return "OK";
  }
}

} // namespace COBS
