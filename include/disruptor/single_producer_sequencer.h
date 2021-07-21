//
// Created by shawnfeng on 7/11/21.
// Copyright (c) 2021 shawnfeng. All rights reserved.
//
#pragma once

#include <disruptor/event_cursor.h>

#include <thread>

namespace disruptor {

/**
 *  Tracks the write position in a buffer.
 *
 *  Write cursors need to know the size of the buffer
 *  in order to know how much space is available.
 */
class single_producer_sequencer : public event_cursor {
 public:
  /** @param s - the size of the ringbuffer,
   *  required to do proper wrap detection
   **/
  explicit single_producer_sequencer(int64_t s) : size_(s) {
    next_value_ = 0;
    cached_min_sequence_ = 0;
  }

  /**
   *   We need to wait until the available space in
   *   the ring buffer is  pos - cursor which means that
   *   all readers must be at least to pos - size_ and
   *   that our new end is the min of the readers + size_
   */
  int64_t wait_for(int64_t pos) {
    return barrier_.wait_for(pos - size_) + size_ + 1;
  }

  int64_t next(int64_t num = 1) {
    if (num < 1 || num > size_)
      throw std::runtime_error("num must be > 0 and < size");

    auto next_sequence = next_value_ + num;
    auto wrap_point = next_sequence - size_;

    if (cached_min_sequence_ <= wrap_point) {
      auto min_sequence = barrier_.get_min(wrap_point);
      while (min_sequence <= wrap_point) {
        std::this_thread::yield();
        min_sequence = barrier_.get_min(wrap_point);
      }
      cached_min_sequence_ = min_sequence;
    }

    return next_value_ = next_sequence;
  }

 protected:
  const int64_t size_;
  int64_t next_value_;
  int64_t cached_min_sequence_;
};

}  // namespace disruptor
