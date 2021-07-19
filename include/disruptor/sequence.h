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
 *  A sequence number must be padded to prevent false sharing and
 *  access to the sequence number must be protected by memory barriers.
 *
 *  In addition to tracking the sequence number, additional state associated
 *  with the sequence number is also made available.  No false sharing
 *  should occur because all 'state' is only written by one thread. This
 *  extra state includes whether or not this sequence number is 'EOF' and
 *  whether or not any alerts have been published.
 */
class sequence {
 public:
  explicit sequence(int64_t v = 0) : _sequence(v), _alert(false) {}

  int64_t acquire() const { return _sequence.load(std::memory_order_acquire); }
  void store(int64_t value) {
    _sequence.store(value, std::memory_order_release);
  }

  /** when the cursor hits the end of a stream, it can set the eof flag */
  void set_eof() { _alert = true; }
  bool eof() const { return _alert; }

  int64_t increment_and_get(int64_t inc) {
    return _sequence.fetch_add(inc, std::memory_order_release) + inc;
  }

 private:
  std::atomic<int64_t> _sequence;
  volatile bool _alert;
};

}  // namespace disruptor
