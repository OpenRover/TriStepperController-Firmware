// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
#pragma once

#include <protocol.h>

namespace Global {

extern unsigned rx_count, tx_count;

extern Protocol::RX rx;
extern Protocol::TX tx;

namespace Config {

constexpr long SERIAL_BAUD_RATE = 115200;
extern bool log;

} // namespace Config

} // namespace Global
