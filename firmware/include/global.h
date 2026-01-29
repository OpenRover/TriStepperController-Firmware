// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
#pragma once

#include <protocol.h>
#include <scheduler.h>

namespace Global {

extern Protocol::RX rx;
extern Protocol::TX tx;
extern Scheduler::Scheduler<64> scheduler;

namespace Config {

constexpr long SERIAL_BAUD_RATE = 115200;
extern bool log;

} // namespace Config

} // namespace Global
