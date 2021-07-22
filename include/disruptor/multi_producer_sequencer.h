//
// Created by shawnfeng on 7/11/21.
// Copyright (c) 2021 shawnfeng. All rights reserved.
//
#pragma once

#include <disruptor/event_cursor.h>

namespace disruptor {

/**
 *  When there are multiple writers this cursor can
 *  be used to reserve space in the write buffer
 *  in an atomic manner.
 *
 *  @code
 *  auto start = cur->next(slots);
 *  ... do your writes...
 *  cur->publish_after( start + slots, start -1 );
 *  @endcode
 *
 */
class MultiProducerSequencer : public EventCursor {
 private:
  using EventCursor::publish;

 public:
  /** @param s - the size of the ringbuffer,
   *  required to do proper wrap detection
   **/
  explicit MultiProducerSequencer(int64_t s) : size_(s) {
    cached_min_sequence_ = Sequence::INIT_SEQUENCE;
  }

  /**
   * When there are multiple writers they cannot both assume the right to
   * write, instead they must first next some slots in an atomic manner.
   *
   *  After pos().acquire() == next( slots ) -1 the claimer
   *  is free to call publish up to start + slots -1
   *
   *  @return the first slot the caller may write to.
   */
  int64_t next(int64_t num_slots = 1) {
    if (num_slots < 1 || num_slots > size_) {
      throw std::runtime_error("num must be > 0 and < size");
    }
    auto next_sequence = claim_cursor_.increment_and_get(num_slots);
    auto wrap_point = next_sequence - size_;

    // make sure there is enough space to write
    if (wrap_point >= cached_min_sequence_) {
      int64_t min_sequence;
      while (!eof() &&
             wrap_point >= (min_sequence = barrier_.get_min(wrap_point))) {
        std::this_thread::yield();
      }
      cached_min_sequence_ = min_sequence;
    }

    if (eof()) throw Eof{};

    return next_sequence;
  }

  void publish_after(int64_t pos, int64_t after_pos) {
    assert(pos > after_pos);

    // Wait other producers
    while (!eof() && after_pos > acquire()) {
      // Busy waiting, if other producers claim the sequence, it should complete
      // the release as soon as possible
    }

    if (eof()) throw Eof{};

    publish(pos);
  }

 private:
  const int64_t size_;
  Sequence claim_cursor_;
  int64_t cached_min_sequence_;
};

}  // namespace disruptor
