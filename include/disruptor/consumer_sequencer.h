//
// Created by shawnfeng on 7/11/21.
// Copyright (c) 2021 shawnfeng. All rights reserved.
//
#pragma once

#include <disruptor/eof.h>
#include <disruptor/event_cursor.h>

namespace disruptor {

/**
 *  Tracks the read position in a buffer
 */
class consumer_sequencer : public event_cursor {
 public:
  explicit consumer_sequencer() : event_cursor() {}

  int64_t wait_for(int64_t next_sequence) {
    try {
      return barrier_.wait_for(next_sequence);
    } catch (...) {
      set_eof();
      throw;
    }
  }
};

}  // namespace disruptor
