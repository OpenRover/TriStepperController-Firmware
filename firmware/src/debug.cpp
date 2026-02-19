// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================

#include "debug.h"

#include <Arduino.h>

void __trigger_panic__() {
    // Trigger panic reset
    ESP.restart();
}
