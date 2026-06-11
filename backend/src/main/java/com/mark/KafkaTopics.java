package com.mark;

import org.springframework.boot.context.properties.ConfigurationProperties;

@ConfigurationProperties(prefix = "mark.kafka")
public record KafkaTopics(String jobsTopic, String resultsTopic) {}
