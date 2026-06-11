package com.mark;

import java.util.UUID;
import org.springframework.http.HttpStatus;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.ResponseStatus;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.server.ResponseStatusException;

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
    producer.publish(jobId, request.source());
    return job;
  }

  @GetMapping("/{jobId}")
  public CompileJob get(@PathVariable String jobId) {
    var job = store.get(jobId);
    if (job == null) throw new ResponseStatusException(HttpStatus.NOT_FOUND);
    return job;
  }
}
