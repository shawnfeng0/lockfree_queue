#include <disruptor/disruptor.h>
#include <gtest/gtest.h>
#include <sys/time.h>

#include <iostream>
#include <thread>

// Precompiler define to get only filename;
#if !defined(__FILENAME__)
#define __FILENAME__                                       \
  (strrchr(__FILE__, '/')    ? strrchr(__FILE__, '/') + 1  \
   : strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 \
                             : __FILE__)
#endif

#define LOGGER_WARN std::cout << "(" << __FILENAME__ << ":" << __LINE__ << ") "
#define LOGGER_DEBUG std::cout << "(" << __FILENAME__ << ":" << __LINE__ << ") "

TEST(queue, one_pub_one_sub) {
  static constexpr int64_t kSize = 1024;
  static constexpr int64_t kIterations = 30 * 1000 * 1000;

  auto source_data = std::make_shared<disruptor::ring_buffer<int64_t, kSize>>();
  auto subscriber_cursor = std::make_shared<disruptor::consumer_sequencer>();
  auto publisher_cursor =
      std::make_shared<disruptor::single_producer_sequencer>(
          source_data->size());

  subscriber_cursor->follow(publisher_cursor);
  publisher_cursor->follow(subscriber_cursor);

  // thread publisher
  auto publish_thread = [=] {
    try {
      for (int64_t i = 0; i < kIterations; ++i) {
        auto pos = publisher_cursor->next();
        source_data->at(pos) = pos;
        publisher_cursor->publish(pos);
      }
      publisher_cursor->set_eof();
    } catch (std::exception& e) {
      LOGGER_WARN << "publisher caught: " << e.what() << " at pos "
                  << publisher_cursor->acquire() << "\n";
    }
  };

  auto subscribe_thread = [=] {
    try {
      auto next_sequence = subscriber_cursor->acquire() + 1;
      ASSERT_EQ(next_sequence, 0);
      while (true) {
        auto available_sequence = subscriber_cursor->wait_for(next_sequence);
        while (next_sequence <= available_sequence) {
          ASSERT_EQ(source_data->at(next_sequence), next_sequence);
          ++next_sequence;
        }
        subscriber_cursor->publish(available_sequence);
      }
    } catch (std::exception& e) {
      LOGGER_WARN << "source_subscriber_cursor caught: " << e.what() << "\n";
    }
  };

  struct timespec start_tp {};
  struct timespec end_tp {};

  clock_gettime(CLOCK_MONOTONIC, &start_tp);

  std::thread sub(subscribe_thread);
  std::thread pub(publish_thread);

  pub.join();
  sub.join();

  clock_gettime(CLOCK_MONOTONIC, &end_tp);

  double start =
      (double)(start_tp.tv_sec) + (double)start_tp.tv_nsec / 1000 / 1000 / 1000;
  double end =
      (double)(end_tp.tv_sec) + (double)end_tp.tv_nsec / 1000 / 1000 / 1000;

  LOGGER_DEBUG << "1 producer - 1 consumer performance: "
               << (kIterations * 1.0) / (end - start) / 1000.0 / 1000.0
               << " M ops/secs" << std::endl;
}
