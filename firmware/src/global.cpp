// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
#include "global.h"
#include <Arduino.h>

namespace IO {

bool available() { return Serial.available() > 0; }
char read() { return Serial.read(); }
size_t write(const void *buf, size_t size) {
  return Serial.write(static_cast<const char *>(buf), size);
}
} // namespace IO

Protocol::RX Global::rx(IO::available, IO::read);
Protocol::TX Global::tx(IO::write);

bool Global::Config::log = true;

size_t debug_write(void *buf, size_t size) {
  return Serial2.write(static_cast<const char *>(buf), size);
}
