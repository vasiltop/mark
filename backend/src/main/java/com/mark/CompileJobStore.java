package com.mark;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import org.springframework.stereotype.Component;

@Component
public class CompileJobStore {
  private final Map<String, CompileJob> jobs = new ConcurrentHashMap<>();

  public void save(CompileJob job) {
    jobs.put(job.jobId(), job);
  }

  public CompileJob get(String jobId) {
    return jobs.get(jobId);
  }
}
