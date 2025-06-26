// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================

#pragma once

constexpr unsigned long long operator"" _s(unsigned long long val) {
  return val * 1000000;
}

constexpr unsigned long long operator"" _s(long double val) {
  return static_cast<unsigned long long>(val * 1000000);
}

constexpr unsigned long long operator"" _ms(unsigned long long val) {
  return val * 1000;
}

constexpr unsigned long long operator"" _ms(long double val) {
  return static_cast<unsigned long long>(val * 1000);
}

constexpr unsigned long long operator"" _us(unsigned long long val) {
  return val;
}

constexpr unsigned long long operator"" _us(long double val) {
  return static_cast<unsigned long long>(val);
}
