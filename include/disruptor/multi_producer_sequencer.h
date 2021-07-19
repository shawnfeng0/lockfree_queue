//
// Created by shawnfeng on 7/11/21.
// Copyright (c) 2021 shawnfeng. All rights reserved.
//
#pragma once

#include <disruptor/single_producer_sequencer.h>

namespace disruptor {

/**
 *  When there are multiple writers this cursor can
 *  be used to reserve space in the write buffer
 *  in an atomic manner.
 *
 *  @code
 *  auto start = cur->claim(slots);
 *  ... do your writes...
 *  cur->publish_after( start + slots, start -1 );
 *  @endcode
 *
 */
class multi_producer_sequencer : public single_producer_sequencer {
 public:
  /** @param s - the size of the ringbuffer,
   *  required to do proper wrap detection
   **/
  explicit multi_producer_sequencer(int64_t s) : single_producer_sequencer(s) {}

  /** When there are multiple writers they cannot both
   *  assume the right to write to begin() to end(),
   *  instead they must first claim some slots in an
   *  atomic manner.
   *
   *
   *  After pos().acquire() == claim( slots ) -1 the claimer
   *  is free to call publish up to start + slots -1
   *
   *  @return the first slot the caller may write to.
   */
  int64_t claim(int64_t num_slots) {
    if (num_slots < 1 || num_slots < size_) {
      throw std::runtime_error("num must be > 0 and < size");
    }
    auto pos = _claim_cursor.increment_and_get(num_slots);
    // make sure there is enough space to write
    wait_for(pos);  // TODO: -1????
    return pos - num_slots;
  }

  void publish_after(int64_t pos, int64_t after_pos) {
    try {
      assert(pos > after_pos);
      barrier_.wait_for(after_pos);
      publish(pos);
    } catch (...) {
      set_eof();
      throw;
    }
  }

 private:
  sequence _claim_cursor;
};

}  // namespace disruptor
