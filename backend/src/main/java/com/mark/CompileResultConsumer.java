package com.mark;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.kafka.annotation.KafkaListener;
import org.springframework.stereotype.Component;

@Component
public class CompileResultConsumer {
  private final CompileJobStore store;
  private final ObjectMapper json = new ObjectMapper();

  public CompileResultConsumer(CompileJobStore store) {
    this.store = store;
  }

  @KafkaListener(topics = "${mark.kafka.results-topic}")
  public void onResult(String message) {
    try {
      JsonNode node = json.readTree(message);
      var jobId = node.get("jobId").asText();
      var existing = store.get(jobId);
      if (existing == null) return;

      if (node.hasNonNull("error")) {
        Integer line = node.has("errorLine") ? node.get("errorLine").asInt() : null;
        Integer col = node.has("errorCol") ? node.get("errorCol").asInt() : null;
        store.save(existing.withError(node.get("error").asText(), line, col));
        return;
      }

      store.save(existing.withResult(
          node.path("html").asText(null),
          node.path("css").asText(null)
      ));
    } catch (Exception ignored) {
    }
  }
}
