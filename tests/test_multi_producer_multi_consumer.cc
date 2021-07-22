#include <disruptor/disruptor.h>
#include <gtest/gtest.h>

#include <array>
#include <ctime>
#include <thread>

#include "slog.h"

static constexpr auto kProduceThreadNum = 3;
static constexpr auto kConsumeThreadNum = 3;

TEST(queue, three_producer_three_consumer) {
  static constexpr int64_t kSize = 1024;
  static constexpr int64_t kIterations = kProduceThreadNum * 1000 * 1000;

  auto source_data = std::make_shared<disruptor::RingBuffer<int64_t, kSize>>();
  auto producer_sequence =
      std::make_shared<disruptor::MultiProducerSequencer>(
          source_data->size());
  std::array<std::shared_ptr<disruptor::ConsumerSequencer>, kConsumeThreadNum>
      consumer_sequences;

  for (auto& consumer_sequence : consumer_sequences) {
    consumer_sequence = std::make_shared<disruptor::ConsumerSequencer>();
    producer_sequence->follow(consumer_sequence);
    consumer_sequence->follow(producer_sequence);
  }

  auto produce_thread_entry =
      [=](std::shared_ptr<disruptor::MultiProducerSequencer> producer) {
        try {
          for (int64_t i = 0; i < kIterations; ++i) {
            auto pos = producer->next();
            source_data->at(pos) = pos;
            producer->publish_after(pos, pos - 1);
          }
          producer->set_eof();
        } catch (std::exception& e) {
          LOGGER_WARN("producer caught: %s, at pos %ld", e.what(),
                      producer->acquire());
        }
      };

  auto consume_thread_entry =
      [=](std::shared_ptr<disruptor::ConsumerSequencer> consumer) {
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

  std::array<std::thread, kProduceThreadNum> produce_threads;
  for (auto& p : produce_threads)
    p = std::thread{produce_thread_entry, producer_sequence};

  std::array<std::thread, consumer_sequences.size()> consume_threads;
  for (auto i = 0; i < consume_threads.size(); i++) {
    consume_threads[i] =
        std::thread{consume_thread_entry, consumer_sequences[i]};
  }

  for (auto& p : produce_threads) p.join();
  for (auto& c : consume_threads) c.join();

  clock_gettime(CLOCK_MONOTONIC, &end_tp);

  double start =
      (double)(start_tp.tv_sec) + (double)start_tp.tv_nsec / 1000 / 1000 / 1000;
  double end =
      (double)(end_tp.tv_sec) + (double)end_tp.tv_nsec / 1000 / 1000 / 1000;

  LOGGER_DEBUG("%d producer - %d consumer performance: %f M ops/secs",
               kProduceThreadNum, kConsumeThreadNum,
               producer_sequence->acquire() / (end - start) / 1000.0 / 1000.0);
}
