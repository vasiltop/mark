package com.mark;

import java.time.Instant;

public record CompileJob(
    String jobId,
    String source,
    String status,
    String html,
    String css,
    String error,
    Integer errorLine,
    Integer errorCol,
    Instant createdAt,
    Instant completedAt
) {
  public static CompileJob pending(String jobId, String source) {
    return new CompileJob(jobId, source, "pending", null, null, null, null, null, Instant.now(), null);
  }

  public CompileJob withResult(String html, String css) {
    return new CompileJob(jobId, source, "done", html, css, null, null, null, createdAt, Instant.now());
  }

  public CompileJob withError(String error, Integer errorLine, Integer errorCol) {
    return new CompileJob(jobId, source, "error", null, null, error, errorLine, errorCol, createdAt, Instant.now());
  }
}
