// Emulated function calls for Arduino environment
#pragma once

#include <chrono>
#include <thread>

extern "C" {

unsigned long micros();

unsigned long millis();
}

inline void delay(unsigned long ms) {
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
