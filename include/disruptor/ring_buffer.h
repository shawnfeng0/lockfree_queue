//
// Created by shawnfeng on 7/11/21.
// Copyright (c) 2021 shawnfeng. All rights reserved.
//
#pragma once
#include <unistd.h>

#include <atomic>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

namespace disruptor {

/**
 *  Provides a automatic index into a ringbuffer with
 *  a power of 2 size.
 */
template <typename EventType, uint64_t Size = 1024>
class ring_buffer {
 public:
  typedef EventType event_type;

  static_assert(((Size != 0) && ((Size & (~Size + 1)) == Size)),
                "Ring buffer's must be a power of 2");

  /** @return a read-only reference to the event at pos */
  const EventType& at(int64_t pos) const { return _buffer[pos & (Size - 1)]; }
  const EventType& operator[](int64_t pos) const {
    return _buffer[pos & (Size - 1)];
  }

  /** @return a reference to the event at pos */
  EventType& at(int64_t pos) { return _buffer[pos & (Size - 1)]; }
  EventType& operator[](int64_t pos) { return _buffer[pos & (Size - 1)]; }

  /** useful to check for contiguous ranges when EventType is
   *  POD and memcpy can be used.  OR if the buffer is being used
   *  by a socket dumping raw bytes in.  In which case memcpy
   *  would have to use to ranges instead of 1.
   */
  int64_t index(int64_t pos) const { return pos & (Size - 1); }
  int64_t size() const { return Size; }

 private:
  EventType _buffer[Size];
};

}  // namespace disruptor