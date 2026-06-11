#include "kafka.hpp"
#include "../compiler/compile_api.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include <librdkafka/rdkafka.h>

namespace kafka {

auto env_or(const char *key, const char *fallback) -> const char * {
  auto *value = std::getenv(key);
  return value && *value ? value : fallback;
}

internal auto json_escape(const std::string &input) -> std::string {
  std::string out;
  out.reserve(input.size() + 16);
  for (char c : input) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out.push_back(c); break;
    }
  }
  return out;
}

internal auto extract_json_string(const std::string &json, const char *key) -> std::string {
  auto needle = std::string("\"") + key + "\":\"";
  auto pos = json.find(needle);
  if (pos == std::string::npos) return {};
  pos += needle.size();
  std::string out;
  for (; pos < json.size(); pos++) {
    if (json[pos] == '\\' && pos + 1 < json.size()) {
      out.push_back(json[pos + 1]);
      pos++;
      continue;
    }
    if (json[pos] == '"') break;
    out.push_back(json[pos]);
  }
  return out;
}

internal auto publish_result(rd_kafka_t *producer, const char *topic, const std::string &payload) -> bool {
  return rd_kafka_producev(
    producer,
    RD_KAFKA_V_TOPIC(topic),
    RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
    RD_KAFKA_V_VALUE(const_cast<char *>(payload.c_str()), payload.size()),
    RD_KAFKA_V_END
  ) == 0;
}

internal auto handle_job(rd_kafka_t *producer, const Config &config, const std::string &payload) -> void {
  auto job_id = extract_json_string(payload, "jobId");
  auto source = extract_json_string(payload, "source");
  if (job_id.empty()) return;

  auto result = compile::compile_mark(source.c_str(), (s32)source.size());
  std::string out;
  if (!result.ok) {
    out = "{\"jobId\":\"" + json_escape(job_id) + "\",\"error\":\"" +
      json_escape(result.error.msg) + "\"}";
  } else {
    out = "{\"jobId\":\"" + json_escape(job_id) + "\",\"html\":\"" +
      json_escape(result.html) + "\",\"css\":\"" + json_escape(result.css) + "\"}";
  }

  publish_result(producer, config.results_topic, out);
  rd_kafka_poll(producer, 0);
}

auto run_worker(const Config &config) -> int {
  char errstr[512];

  auto *conf = rd_kafka_conf_new();
  rd_kafka_conf_set(conf, "bootstrap.servers", config.brokers, errstr, sizeof(errstr));
  rd_kafka_conf_set(conf, "group.id", config.group_id, errstr, sizeof(errstr));
  rd_kafka_conf_set(conf, "auto.offset.reset", "earliest", errstr, sizeof(errstr));

  auto *consumer = rd_kafka_new(RD_KAFKA_CONSUMER, conf, errstr, sizeof(errstr));
  if (!consumer) {
    std::cerr << "kafka consumer: " << errstr << '\n';
    return 1;
  }

  auto *prod_conf = rd_kafka_conf_new();
  rd_kafka_conf_set(prod_conf, "bootstrap.servers", config.brokers, errstr, sizeof(errstr));
  auto *producer = rd_kafka_new(RD_KAFKA_PRODUCER, prod_conf, errstr, sizeof(errstr));
  if (!producer) {
    std::cerr << "kafka producer: " << errstr << '\n';
    rd_kafka_destroy(consumer);
    return 1;
  }

  rd_kafka_poll_set_consumer(consumer);
  auto *topics = rd_kafka_topic_partition_list_new(1);
  rd_kafka_topic_partition_list_add(topics, config.jobs_topic, RD_KAFKA_PARTITION_UA);
  if (rd_kafka_subscribe(consumer, topics) != RD_KAFKA_RESP_ERR_NO_ERROR) {
    std::cerr << "kafka subscribe failed\n";
    rd_kafka_topic_partition_list_destroy(topics);
    rd_kafka_destroy(producer);
    rd_kafka_destroy(consumer);
    return 1;
  }
  rd_kafka_topic_partition_list_destroy(topics);

  std::clog << "mark-worker listening on " << config.jobs_topic << '\n';

  while (true) {
    auto *message = rd_kafka_consumer_poll(consumer, 1000);
    if (!message) continue;
    if (message->err) {
      rd_kafka_message_destroy(message);
      continue;
    }
    std::string payload(static_cast<const char *>(message->payload), message->len);
    handle_job(producer, config, payload);
    rd_kafka_message_destroy(message);
  }
}

} // namespace kafka
