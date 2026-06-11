package com.mark;

import com.fasterxml.jackson.databind.ObjectMapper;
import java.util.HashMap;
import java.util.Map;
import org.springframework.kafka.core.KafkaTemplate;
import org.springframework.stereotype.Component;

@Component
public class CompileJobProducer {
  private final KafkaTemplate<String, String> kafka;
  private final KafkaTopics topics;
  private final AssetStore assets;
  private final ObjectMapper json = new ObjectMapper();

  public CompileJobProducer(KafkaTemplate<String, String> kafka, KafkaTopics topics, AssetStore assets) {
    this.kafka = kafka;
    this.topics = topics;
    this.assets = assets;
  }

  public void publish(String jobId, CreateJobRequest request) {
    try {
      var payload = new HashMap<String, Object>();
      payload.put("jobId", jobId);
      payload.put("source", request.source());
      if (request.context() != null && !request.context().isEmpty()) {
        payload.put("context", request.context());
      }
      if (request.assets() != null && !request.assets().isEmpty()) {
        var resolved = new HashMap<String, String>();
        for (var entry : request.assets().entrySet()) {
          var asset = assets.get(entry.getValue());
          if (asset != null) resolved.put(entry.getKey(), asset.dataBase64());
        }
        if (!resolved.isEmpty()) payload.put("assets", resolved);
      }
      kafka.send(topics.jobsTopic(), jobId, json.writeValueAsString(payload));
    } catch (Exception e) {
      throw new RuntimeException("failed to publish compile job", e);
    }
  }
}
