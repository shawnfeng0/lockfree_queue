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
class SingleProducerSequencer : public EventCursor {
 public:
  /** @param s - the size of the ringbuffer,
   *  required to do proper wrap detection
   **/
  explicit SingleProducerSequencer(int64_t s) : size_(s) {
    next_sequence_ = Sequence::INIT_SEQUENCE;
    cached_min_sequence_ = Sequence::INIT_SEQUENCE;
  }

  int64_t next(int64_t num = 1) {
    if (num < 1 || num > size_)
      throw std::runtime_error("num must be > 0 and < size");

    next_sequence_ += num;
    auto wrap_point = next_sequence_ - size_;

    // make sure there is enough space to write
    if (wrap_point >= cached_min_sequence_) {
      int64_t min_sequence;
      while (wrap_point >= (min_sequence = barrier_.get_min(wrap_point))) {
        std::this_thread::yield();
      }
      cached_min_sequence_ = min_sequence;
    }

    return next_sequence_;
  }

 protected:
  const int64_t size_;
  int64_t next_sequence_;
  int64_t cached_min_sequence_;
};

}  // namespace disruptor
