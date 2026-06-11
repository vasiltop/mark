#include <iostream>

#include "../compiler/compile.hpp"
#include "kafka.hpp"

#include "kafka.cpp"

auto main() -> int {
  kafka::Config config {
    .brokers = kafka::env_or("KAFKA_BOOTSTRAP", "localhost:9092"),
    .jobs_topic = kafka::env_or("KAFKA_JOBS_TOPIC", "compile.jobs"),
    .results_topic = kafka::env_or("KAFKA_RESULTS_TOPIC", "compile.results"),
    .group_id = kafka::env_or("KAFKA_GROUP_ID", "mark-worker"),
  };
  return kafka::run_worker(config);
}
