#pragma once

namespace kafka {

struct Config {
  const char *brokers;
  const char *jobs_topic;
  const char *results_topic;
  const char *group_id;
};

auto env_or(const char *key, const char *fallback) -> const char *;
auto run_worker(const Config &config) -> int;

} // namespace kafka
