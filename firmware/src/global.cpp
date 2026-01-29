// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
#include "global.h"
#include "board.h"
#include "utils.h"
#include <Arduino.h>

namespace IO {
bool available() { return Serial.available() > 0; }
char read() { return static_cast<char>(Serial.read()); }
size_t write(const void *buf, size_t size) {
  return Serial.write(static_cast<const char *>(buf), size);
}
} // namespace IO

Protocol::RX Global::rx(IO::available, IO::read);
Protocol::TX Global::tx(IO::write);

void update_status(Scheduler::Status s) {
  switch (s) {
  case Scheduler::STATUS_IDLE:
    Board::LED::BUILTIN.write(LOW);
    break;
  case Scheduler::STATUS_BUSY:
    Board::LED::BUILTIN.write(HIGH);
    break;
  default:
    LOG("Unknown scheduler status: %d", s);
    Board::LED::RED.write(HIGH);
  }
};

Scheduler::Scheduler<64> Global::scheduler(update_status);

bool Global::Config::log = true;
