#include <disruptor/disruptor.h>
#include <gtest/gtest.h>
#include <sys/time.h>

#include <iostream>
#include <thread>

#include "slog.h"

TEST(queue, one_producer_three_consumer) {
  static constexpr int64_t kSize = 1024;
  static constexpr int64_t kIterations = 30 * 1000 * 1000;

  auto source_data = std::make_shared<disruptor::ring_buffer<int64_t, kSize>>();
  auto producer_sequence =
      std::make_shared<disruptor::single_producer_sequencer>(
          source_data->size());
  auto consumer_sequence1 = std::make_shared<disruptor::consumer_sequencer>();
  auto consumer_sequence2 = std::make_shared<disruptor::consumer_sequencer>();
  auto consumer_sequence3 = std::make_shared<disruptor::consumer_sequencer>();

  producer_sequence->follow(consumer_sequence1);
  producer_sequence->follow(consumer_sequence2);
  producer_sequence->follow(consumer_sequence3);

  consumer_sequence1->follow(producer_sequence);
  consumer_sequence2->follow(producer_sequence);
  consumer_sequence3->follow(producer_sequence);

  auto produce_thread = [=] {
    try {
      for (int64_t i = 0; i < kIterations; ++i) {
        auto pos = producer_sequence->next();
        source_data->at(pos) = pos;
        producer_sequence->publish(pos);
      }
      producer_sequence->set_eof();
    } catch (std::exception& e) {
      LOGGER_WARN("producer caught: %s, at pos %ld", e.what(),
                  producer_sequence->acquire());
    }
  };

  auto consume_thread =
      [=](std::shared_ptr<disruptor::consumer_sequencer> consumer) {
        try {
          auto next_sequence = consumer->acquire() + 1;
          ASSERT_EQ(next_sequence, 0);
          while (true) {
            auto available_sequence = consumer->wait_for(next_sequence);
            while (next_sequence <= available_sequence) {
              ASSERT_EQ(source_data->at(next_sequence), next_sequence);
              ++next_sequence;
            }
            consumer->publish(available_sequence);
          }
        } catch (std::exception& e) {
          LOGGER_WARN("consumer caught: %s, at pos %ld", e.what(),
                      consumer->acquire());
        }
      };

  struct timespec start_tp {};
  struct timespec end_tp {};

  clock_gettime(CLOCK_MONOTONIC, &start_tp);

  std::thread p1(produce_thread);
  std::thread c1(consume_thread, consumer_sequence1);
  std::thread c2(consume_thread, consumer_sequence2);
  std::thread c3(consume_thread, consumer_sequence3);

  p1.join();
  c1.join();
  c2.join();
  c3.join();

  clock_gettime(CLOCK_MONOTONIC, &end_tp);

  double start =
      (double)(start_tp.tv_sec) + (double)start_tp.tv_nsec / 1000 / 1000 / 1000;
  double end =
      (double)(end_tp.tv_sec) + (double)end_tp.tv_nsec / 1000 / 1000 / 1000;

  LOGGER_DEBUG("1 producer - 3 consumer performance: %f M ops/secs",
               kIterations / (end - start) / 1000.0 / 1000.0);
}
