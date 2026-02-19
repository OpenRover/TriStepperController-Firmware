// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================
#pragma once

#include <cstddef>

// Lock-free, interrupt-safe single-producer single-consumer ring buffer
// Requirements:
// - SIZE must be a power of 2
// - Producer (main thread) calls: writable(), write()
// - Consumer (ISR) calls: readable(), peek(), pop()
template <typename T, size_t S> class RingBuffer {
private:
  T buffer[S];
  volatile size_t head = 0; // Producer writes, consumer reads (next write pos)
  volatile size_t tail = 0; // Consumer writes, producer reads (next read pos)
  const size_t mask;        // SIZE - 1, for fast modulo with power-of-2

  // Compile-time check that SIZE is power of 2
  static constexpr bool is_power_of_2(size_t n) {
    return n > 0 && (n & (n - 1)) == 0;
  }

public:
  inline RingBuffer() : mask(S - 1) {
    static_assert(is_power_of_2(S), "RingBuffer size must be a power of 2");
  }

  inline unsigned len() const {
    // Warning: not atomic, may be inconsistent if head/tail change during call
    return (head - tail) & mask;
  }

  // Consumer methods (called from ISR)
  inline bool readable() const {
    // Read head (written by producer) - this is safe because producer only
    // increments head
    return head != tail;
  }
  inline T &peek() {
    // Must only call after readable() returns true
    return buffer[tail & mask];
  }
  inline void pop() {
    // Update tail - this makes one more slot writable for producer
    // Use masking instead of modulo for efficiency
    // Memory barrier ensures data read completes before tail update
    asm volatile("" ::: "memory");
    tail = (tail + 1) & mask;
  }

  // Producer methods (called from main thread)
  inline bool writable() const {
    // Read tail (written by consumer) - safe because consumer only increments
    // tail Check if next write position would collide with tail
    return ((head + 1) & mask) != (tail & mask);
  }

  inline void push(const T &item) {
    // Must only call after writable() returns true
    // CRITICAL: Write data BEFORE updating head index
    // This ensures consumer never sees uninitialized data
    buffer[head & mask] = item;
    // Memory barrier ensures data write completes before head update
    asm volatile("" ::: "memory");
    head = (head + 1) & mask;
  }
};
