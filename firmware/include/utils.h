// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================

#pragma once

#include <cstddef>
#include <cstdio>

#include "debug.h"
#include "global.h"

auto constexpr LOG_BUF_SIZE = sizeof(Global::tx.frame.payload);

#define LOG(T, ...)                                                            \
  if (Global::Config::log) {                                                   \
    __printf__(snprintf((char *)&Global::tx.frame.payload, LOG_BUF_SIZE, T,    \
                        __VA_ARGS__));                                         \
  }

#define WARN(T, ...)                                                           \
  __printf__(snprintf((char *)&Global::tx.frame.payload, LOG_BUF_SIZE,         \
                      "[WARN] " T, __VA_ARGS__))

inline void __printf__(int ret) {
  if (ret > 0) {
    ret = ret >= static_cast<int>(LOG_BUF_SIZE) ? LOG_BUF_SIZE - 1 : ret;
    Global::tx.frame.header.set(0, Protocol::Method::LOG);
    Global::tx.frame.payload_size = ret;
    Global::tx.encode_frame();
    Global::tx.send_frame();
  }
}

inline void NO_NULL(void *ptr) {
  if (!ptr) {
    PANIC("NULL pointer encountered");
  }
}

template <unsigned long ns> inline void delay_ns() {
  constexpr unsigned long cycles = ns / (1000000000 / F_CPU);
  for (unsigned long i = 0; i < cycles; i++) {
    asm volatile("nop");
  }
}
