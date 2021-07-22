//
// Created by shawnfeng on 7/11/21.
// Copyright (c) 2021 shawnfeng. All rights reserved.
//
#pragma once

#include <disruptor/barrier.h>
#include <disruptor/sequence.h>

namespace disruptor {

/**
 *  A cursor is used to track the location of a publisher / subscriber within
 *  the ring buffer.  Cursors track a range of entries that are waiting
 *  to be processed.  After a cursor is 'done' with an entry it can publish
 *  that fact.
 *
 *  There are two types of cursors, read_cursors and write cursors. read_cursors
 *  block when they need to
 *
 *  Events between [begin,end) may be processed at will for readers.  When a
 reader
 *  is done they can 'publish' their progress which will move begin up to
 *  published position+1.   When begin == end, the cursor must call
 next(end),
 *  next() will return a new 'end'.
 *
 *  @section read_cursor_example Read Cursor Example
 *  @code
      auto source   = std::make_shared<ring_buffer<EventType,SIZE>>();
      auto dest     = std::make_shared<ring_buffer<EventType,SIZE>>();
      auto p        = std::make_shared<single_write_cursor>("write",SIZE)
        //;
      auto a        = std::make_shared<read_cursor>("a");

      a->follow(p);
      p->follow(a);

      auto pos      = a->begin();
      auto end      = a->end();
      while( true )
      {
         if( pos == end )
         {
             a->publish(pos-1);
             end = a->next(end);
         }
         dest->at(pos) = source->at(pos);
         ++pos;
      }
 *  @endcode
 *
 *
 *  @section write_cursor_example Write Cursor Example
 *
 *  The following code would run in the publisher thread.  The
 *  publisher can write data without 'waiting' until it pos is
 *  greater than or equal to end.  The 'initial condition' of
 *  a publisher is with pos > end because the write cursor
 *  cannot 'be valid' for readers until after the first element
 *  is written.
 *
    @code
        auto pos = p->begin();
        auto end = p->end();
        while( !done )
        {
           if( pos >= end )
           {
              end = p->next(end);
           }
           source->at( pos ) = i;
           p->publish(pos);
           ++pos;
        }
        // set eof to signal any followers to stop waiting after
        // they hit this position.
        p->set_eof();
    @endcode
 *
 *
 *
 */
class EventCursor : public Sequence {
 private:
  using Sequence::store;

 public:
  /** this event processor will process every event
   *  upto, but not including s
   */
  template <typename T>
  void follow(T&& s) {
    barrier_.follow(std::forward<T>(s));
  }

  /** makes the event at p available to those following this cursor */
  void publish(int64_t p) { store(p); }

 protected:
  /** last know available, min(_limit_seq) */
  Barrier barrier_;
};

}  // namespace disruptor
