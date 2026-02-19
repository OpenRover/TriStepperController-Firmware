// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================

#pragma once

typedef unsigned long long Micros;

constexpr Micros operator""_s(unsigned long long val) { return val * 1000000; }

constexpr Micros operator""_s(long double val) {
  return static_cast<Micros>(val * 1000000);
}

constexpr Micros operator""_ms(unsigned long long val) { return val * 1000; }

constexpr Micros operator""_ms(long double val) {
  return static_cast<Micros>(val * 1000);
}

constexpr Micros operator""_us(unsigned long long val) { return val; }

constexpr Micros operator""_us(long double val) {
  return static_cast<Micros>(val);
}
