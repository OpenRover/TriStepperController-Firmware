// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================

#include "debug.h"
#include "esp_attr.h"

char debug_log_buffer[DEBUG_LOG_BUFFER_SIZE];

RTC_NOINIT_ATTR __Trace__ __trace_core0__ = {"NA", 0, "NA", NULL, 0};
RTC_NOINIT_ATTR __Trace__ __trace_core1__ = {"NA", 0, "NA", NULL, 1};
