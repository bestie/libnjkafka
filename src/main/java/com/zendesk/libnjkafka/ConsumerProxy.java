package com.zendesk.libnjkafka;

import java.lang.String;
import java.time.Duration;
import java.util.Collection;
import java.util.Properties;
import java.util.Set;
import org.apache.kafka.clients.consumer.ConsumerRecords;
import org.apache.kafka.clients.consumer.KafkaConsumer;
import org.apache.kafka.common.TopicPartition;

public class ConsumerProxy {
  private final KafkaConsumer<String, String> delegate;

  public static ConsumerProxy create(Properties properties) {
    return new ConsumerProxy(new KafkaConsumer<>(mergeDefaultConfig(properties)));
  }

  public ConsumerProxy(KafkaConsumer<String, String> delegate) {
    this.delegate = delegate;
  }

  public Set<TopicPartition> assignment() {
    return delegate.assignment();
  }

  public void close() {
    delegate.close();
  }

  public void commitSync(Duration arg0) {
    delegate.commitSync(arg0);
  }

  public void subscribe(Collection<String> arg0) {
    delegate.subscribe(arg0);
  }

  public ConsumerRecords<String, String> poll(Duration arg0) {
    return delegate.poll(arg0);
  }

  private static Properties mergeDefaultConfig(Properties configuredProps) {
    Properties props = defaultProperties();
    props.putAll(configuredProps);
    return props;
  }

  public static Properties defaultProperties() {
    Properties props = new Properties();
    props.put("bootstrap.servers", "localhost:9092");
    props.put("enable.auto.commit", "true");
    props.put("auto.commit.interval.ms", "1000");
    props.put("key.deserializer", "org.apache.kafka.common.serialization.StringDeserializer");
    props.put("value.deserializer", "org.apache.kafka.common.serialization.StringDeserializer");
    props.put("auto.offset.reset", "earliest");
    return props;
  }
}
