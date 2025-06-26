// =============================================================================
// Stepper Controller Firmware for the RoverMaster Project
// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// ============================================================================
#include "agent.h"
#include "emulator.h"
#include "global.h"
#include "utils.h"
#include <iostream>

namespace io {
size_t write(const void *buf, size_t size) {
  return std::cout.write(static_cast<const char *>(buf), size).tellp();
}
bool available() { return std::cin.rdbuf()->in_avail() > 0; }
char read() { return static_cast<char>(std::cin.get()); }
} // namespace io

Protocol::RX Global::rx(io::available, io::read);
Protocol::TX Global::tx(io::write);
Scheduler::Scheduler<64> Global::scheduler;
bool Global::Config::log = true;

class Blink : public Scheduler::Task {
public:
  unsigned long last_tick = micros();
  Blink() : Scheduler::Task(Scheduler::Task::Recurrent{0.5_s}) {}
  void tick(Scheduler::Micros now) override {
    LOG("Blink task executed, interval = %f s\n",
        (double)(now - last_tick) / (double)1_s);
    last_tick = now;
  }
} blink_task;

const unsigned long epoch = micros();

Scheduler::Scheduler<> sch;

int main(int, const char *[]) {
  try {
    Global::scheduler.tasks.clear().add(agent).add(blink_task);
    Global::scheduler.loop();
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return -1;
  }
}

void __panic_enter__() { std::cerr << "Entering panic mode..." << std::endl; }

void __panic_signal__() { std::cerr << "Panic signal triggered!" << std::endl; }
