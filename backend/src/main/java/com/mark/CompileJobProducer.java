package com.mark;

import com.fasterxml.jackson.databind.ObjectMapper;
import java.util.Map;
import org.springframework.kafka.core.KafkaTemplate;
import org.springframework.stereotype.Component;

@Component
public class CompileJobProducer {
  private final KafkaTemplate<String, String> kafka;
  private final KafkaTopics topics;
  private final ObjectMapper json = new ObjectMapper();

  public CompileJobProducer(KafkaTemplate<String, String> kafka, KafkaTopics topics) {
    this.kafka = kafka;
    this.topics = topics;
  }

  public void publish(String jobId, String source) {
    try {
      var payload = json.writeValueAsString(Map.of("jobId", jobId, "source", source));
      kafka.send(topics.jobsTopic(), jobId, payload);
    } catch (Exception e) {
      throw new RuntimeException("failed to publish compile job", e);
    }
  }
}
