// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================

#pragma once
#include "freertos/FreeRTOS.h" // IWYU pragma: export
#include "stdio.h"

extern char debug_log_buffer[];

size_t debug_write(void *buf, size_t size);

#define DEBUG_LOG_BUFFER_SIZE 1024
#define DEBUG(...)                                                             \
  {                                                                            \
    int __debug_ret__ =                                                        \
        snprintf(debug_log_buffer, DEBUG_LOG_BUFFER_SIZE, __VA_ARGS__);        \
    if (__debug_ret__ > 0) {                                                   \
      debug_write(debug_log_buffer, __debug_ret__);                            \
    }                                                                          \
  }

struct __Trace__ {
  const char *file;
  unsigned line;
  const char *func;
  const char *msg; // Maybe NULL
  char msg_buf[256];
};

extern __Trace__ __trace_core0__;
extern __Trace__ __trace_core1__;

#define TRACE(...)                                                             \
  do {                                                                         \
    const int __core_id__ = xPortGetCoreID();                                  \
    __Trace__ &__trace__ =                                                     \
        (__core_id__ == 0) ? __trace_core0__ : __trace_core1__;                \
    __trace__.file = __FILE__;                                                 \
    __trace__.line = __LINE__;                                                 \
    __trace__.func = __FUNCTION__;                                             \
    __trace__.msg = NULL;                                                      \
    __VA_OPT__(__trace__.msg = __VA_ARGS__);                                   \
  } while (0)

#define TRACE_PRINT(...)                                                       \
  do {                                                                         \
    const int __core_id__ = xPortGetCoreID();                                  \
    __Trace__ &__trace__ =                                                     \
        (__core_id__ == 0) ? __trace_core0__ : __trace_core1__;                \
    __trace__.file = __FILE__;                                                 \
    __trace__.line = __LINE__;                                                 \
    __trace__.func = __FUNCTION__;                                             \
    int __trace_snprintf_ret__ =                                               \
        snprintf(__trace__.msg_buf, sizeof(__trace__.msg_buf), __VA_ARGS__);   \
    if (__trace_snprintf_ret__ > 0) {                                          \
      __trace__.msg = __trace__.msg_buf;                                       \
    } else {                                                                   \
      __trace__.msg = NULL;                                                    \
    }                                                                          \
  } while (0)

#define TRACE_EXIT()                                                           \
  do {                                                                         \
    const int __core_id__ = xPortGetCoreID();                                  \
    __Trace__ &__trace__ =                                                     \
        (__core_id__ == 0) ? __trace_core0__ : __trace_core1__;                \
    __trace__.file = "NA";                                                     \
    __trace__.line = 0;                                                        \
    __trace__.func = "NA";                                                     \
    __trace__.msg = NULL;                                                      \
  } while (0)

void __trigger_panic__();

#define PANIC(...)                                                             \
  do {                                                                         \
    TRACE(__VA_ARGS__);                                                        \
    __trigger_panic__();                                                       \
  } while (0)
