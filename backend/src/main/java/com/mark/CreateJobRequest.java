package com.mark;

import java.util.Map;

public record CreateJobRequest(
    String source,
    Map<String, String> assets,
    Map<String, Object> context
) {}
