// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
#pragma once

#include "global.h"

#include <protocol.h>
#include <scheduler.h>

class Agent : public Scheduler::Task {
public:
  Protocol::RX &rx;
  Protocol::TX &tx;
  inline Agent(Protocol::RX &rx, Protocol::TX &tx)
      : Scheduler::Task(Scheduler::Task::Micro{}), rx(rx), tx(tx) {};
  void tick(Scheduler::Micros) override;
};

extern Agent agent;
