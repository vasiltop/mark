package com.mark;

import java.io.IOException;
import java.util.UUID;
import org.springframework.http.HttpStatus;
import org.springframework.http.MediaType;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.ResponseStatus;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.server.ResponseStatusException;
import org.springframework.web.servlet.mvc.method.annotation.SseEmitter;

@RestController
@RequestMapping("/api/jobs")
public class CompileJobController {
  private final CompileJobStore store;
  private final CompileJobProducer producer;

  public CompileJobController(CompileJobStore store, CompileJobProducer producer) {
    this.store = store;
    this.producer = producer;
  }

  @PostMapping
  @ResponseStatus(HttpStatus.ACCEPTED)
  public CompileJob create(@RequestBody CreateJobRequest request) {
    if (request.source() == null || request.source().isBlank()) {
      throw new ResponseStatusException(HttpStatus.BAD_REQUEST, "source is required");
    }
    var jobId = UUID.randomUUID().toString();
    var job = CompileJob.pending(jobId, request.source());
    store.save(job);
    producer.publish(jobId, request);
    return job;
  }

  @GetMapping("/{jobId}")
  public CompileJob get(@PathVariable String jobId) {
    var job = store.get(jobId);
    if (job == null) throw new ResponseStatusException(HttpStatus.NOT_FOUND);
    return job;
  }

  @GetMapping(value = "/{jobId}/events", produces = MediaType.TEXT_EVENT_STREAM_VALUE)
  public SseEmitter events(@PathVariable String jobId) {
    var existing = store.get(jobId);
    if (existing == null) throw new ResponseStatusException(HttpStatus.NOT_FOUND);

    var emitter = new SseEmitter(60_000L);
    var thread = new Thread(() -> {
      try {
        for (int i = 0; i < 120; i++) {
          var job = store.get(jobId);
          if (job == null) {
            emitter.completeWithError(new ResponseStatusException(HttpStatus.NOT_FOUND));
            return;
          }
          emitter.send(SseEmitter.event().name("status").data(job));
          if ("done".equals(job.status()) || "error".equals(job.status())) {
            emitter.complete();
            return;
          }
          Thread.sleep(500);
        }
        emitter.completeWithError(new ResponseStatusException(HttpStatus.REQUEST_TIMEOUT, "timed out"));
      } catch (IOException | InterruptedException e) {
        emitter.completeWithError(e);
      }
    });
    thread.setDaemon(true);
    thread.start();
    return emitter;
  }
}
