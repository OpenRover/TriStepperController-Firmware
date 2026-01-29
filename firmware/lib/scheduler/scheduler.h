// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================

#pragma once

#include <cstddef>
#include <string>

#include <static-list.h>

#include "duration_literals.h"

extern "C" {
unsigned long millis();
unsigned long micros();
}

namespace Scheduler {
typedef unsigned long Micros;

class Task {
public:
  typedef enum Type : uint8_t {
    TASK_NONE = 0,
    TASK_RECURRENT = 1,
    TASK_ONCE = 2,
    TASK_MICRO = 3,
  } Type;

  typedef struct Recurrent {
    Micros period;
  } Recurrent;

  typedef struct Once {
    bool pending;
  } Once;

  typedef struct Micro {
    // No configurable parameters, just a marker
    // Micro tasks are executed after each tick of other types of tasks.
  } Micro;

  virtual void tick(Micros now) = 0;

  const Type task_type;
  Micros next_tick;
  bool tick_pending = false;
  union {
    Recurrent task_recurrent;
    Once task_once;
    Micro task_micro;
  };

  Task() = delete;
  Task(Recurrent data, Micros startup_delay = 1_ms)
      : task_type(TASK_RECURRENT), next_tick(micros() + startup_delay),
        task_recurrent(data) {};
  Task(Once data)
      : task_type(TASK_ONCE), next_tick(micros()), task_once(data) {};
  Task(Once data, Micros next_tick)
      : task_type(TASK_ONCE), next_tick(next_tick), task_once(data) {};
  Task(Micro) : task_type(TASK_MICRO) {};

  inline void sleep(Scheduler::Micros now, Scheduler::Micros delay) {
    if (task_type != TASK_ONCE)
      throw std::runtime_error("Task::sleep() can only be used with TASK_ONCE");
    task_once.pending = true;
    next_tick = now + delay;
  }
  inline void schedule(Scheduler::Micros tp) {
    if (task_type != TASK_ONCE)
      throw std::runtime_error("Task::sleep() can only be used with TASK_ONCE");
    task_once.pending = true;
    next_tick = tp;
  }
};

typedef enum Status : uint8_t {
  STATUS_IDLE = 0,
  STATUS_BUSY = 1,
} Status;

template <size_t N = 64, Micros WINDOW = 10> class Scheduler {
public:
  // Optional Indicators
  void (*status)(Status) = nullptr;
  struct Perf_s {
    unsigned long since = micros();
    unsigned long busy = 0;
    unsigned long loops = 0;
    double utilization(unsigned long duration) {
      if (duration == 0)
        return 1.0;
      return (double)busy / (double)duration;
    }
    double frequency(unsigned long duration) {
      if (duration == 0)
        return 0.0;
      return 1_ms * (double)loops / (double)duration;
    }
    void reset() {
      since = micros();
      busy = 0;
      loops = 0;
    }
  } perf;
  List<Task, N> tasks;
  Scheduler() = default;
  inline Scheduler(void (*status)(Status)) : status(status) {};
  void loop();
};

template <size_t N, Micros WINDOW> void Scheduler<N, WINDOW>::loop() {
  static Task *next_task;
  while (true) {
    const Micros now = micros();
    const Micros ddl = now + WINDOW;
    // Prepare tick marker
    for (auto &task : tasks) {
      switch (task.task_type) {
      case Task::TASK_ONCE:
        task.tick_pending = task.task_once.pending && (task.next_tick < ddl);
        break;
      case Task::TASK_RECURRENT:
        task.tick_pending = (task.next_tick < ddl);
        break;
      case Task::TASK_MICRO:
        // Micro tasks do not participate in task scheduling.
        task.tick_pending = false;
        break;
      case Task::TASK_NONE:
      default:
        throw std::runtime_error("Scheduler::loop(): Unexpected task type " +
                                 std::to_string(task.task_type));
      }
    }
    do {
      next_task = nullptr;
      // Select next most urgent task
      for (auto &task : tasks) {
        if (!task.tick_pending)
          continue;
        if (next_task == nullptr)
          next_task = &task;
        else if (task.next_tick < next_task->next_tick)
          next_task = &task;
      }
      // Execute selected task (if available)
      if (next_task != nullptr) {
        // Allow tick() to schedule it's own next tick
        switch (next_task->task_type) {
        case Task::TASK_ONCE:
          next_task->task_once.pending = false;
          break;
        case Task::TASK_RECURRENT:
          next_task->next_tick += next_task->task_recurrent.period;
          break;
        default:
          throw std::runtime_error("Scheduler::loop(): Unexpected task type " +
                                   std::to_string(next_task->task_type));
        }
        next_task->tick(now);
        next_task->tick_pending = false;
      }
      perf.busy += micros() - now;
      // Execute recurrent microtasks
      for (auto &task : tasks) {
        if (task.task_type == Task::TASK_MICRO)
          task.tick(micros());
      }
      perf.loops++;
    } while (next_task != nullptr);
  }
};
} // namespace Scheduler
