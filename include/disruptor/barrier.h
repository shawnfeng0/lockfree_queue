//
// Created by shawnfeng on 7/11/21.
// Copyright (c) 2021 shawnfeng. All rights reserved.
//
#pragma once

#include <disruptor/eof.h>
#include <disruptor/sequence.h>

#include <memory>
#include <thread>
#include <vector>

namespace disruptor {

/**
 *   A barrier will block until all cursors it is following are
 *   have moved past a given position.  The barrier uses a
 *   progressive backoff strategy of busy waiting for 1000
 *   tries, yielding for 1000 tries, and the usleeping in 10 ms
 *   intervals.
 *
 *   No wait conditions or locks are used because they would
 *   be 'intrusive' to publishers which must check to see whether
 *   or not they must 'notify'.  The progressive backoff approach
 *   uses little CPU and is a good compromise for most use cases.
 */
class Barrier {
 public:
  void follow(std::shared_ptr<const Sequence> e) {
    limit_seq_.push_back(std::move(e));
  }

  /**
   *  Used to check how much you can read/write without blocking.
   *
   *  @return the min position of every cusror this barrier follow.
   */
  int64_t get_min(int64_t pos) {
    if (last_min_ > pos) return last_min_;

    int64_t min_pos = 0x7fffffffffffffff;
    for (auto& itr : limit_seq_) {
      auto itr_pos = itr->acquire();
      if (itr_pos < min_pos) min_pos = itr_pos;
    }
    return last_min_ = min_pos;
  }

  /*
   *  This method will wait until all s in seq >= pos using a progressive
   *  backoff of busy wait, yield, and usleep(10*1000)
   *
   *  @return the minimum value of every dependency
   */
  int64_t wait_for(int64_t pos) const {
    if (last_min_ > pos) return last_min_;

    int64_t min_pos = 0x7fffffffffffffff;
    for (const auto& itr : limit_seq_) {
      int64_t itr_pos = itr->acquire();

      // yield for a while, queue slowing down
      for (int y = 0; itr_pos < pos && y < 10000; ++y) {
        std::this_thread::yield();
        itr_pos = itr->acquire();
        if (itr->eof()) break;
      }

      // queue stalled, don't peg the CPU but don't wait
      // too long either...
      while (itr_pos < pos) {
        usleep(10 * 1000);
        itr_pos = itr->acquire();
        if (itr->eof()) break;
      }

      if (itr->eof()) {
        if (itr_pos > pos)
          return itr_pos - 1;  // process everything up to itr_pos
        throw Eof();
      }

      if (itr_pos < min_pos) min_pos = itr_pos;
    }
    assert(min_pos != 0x7fffffffffffffff);
    return last_min_ = min_pos;
  }

 private:
  mutable int64_t last_min_{};
  std::vector<std::shared_ptr<const Sequence>> limit_seq_;
};

}  // namespace disruptor
