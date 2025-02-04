package com.zendesk.libnjkafka;

import java.lang.String;
import java.time.Duration;
import java.util.Collection;
import java.util.Properties;
import java.util.Set;
import java.util.Map;

import org.apache.kafka.clients.consumer.ConsumerConfig;
import org.apache.kafka.clients.consumer.ConsumerRebalanceListener;
import org.apache.kafka.clients.consumer.ConsumerRecords;
import org.apache.kafka.clients.consumer.KafkaConsumer;
import org.apache.kafka.clients.consumer.OffsetAndMetadata;
import org.apache.kafka.common.TopicPartition;
import org.graalvm.nativeimage.IsolateThread;

public class ConsumerProxy {
  private final KafkaConsumer<String, String> delegate;
  private IsolateThread pollingThread;

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

  public Map<TopicPartition, OffsetAndMetadata> committed(Set<TopicPartition> partitions, Duration timeout) {
    return delegate.committed(partitions, timeout);
  }

  public void subscribe(Collection<String> arg0, ConsumerRebalanceListener arg1) {
    delegate.subscribe(arg0, arg1);
  }

  public ConsumerRecords<String, String> poll(Duration arg0) {
    return delegate.poll(arg0);
  }

  private static Properties mergeDefaultConfig(Properties configuredProps) {
    Properties props = defaultProperties();
    props.putAll(configuredProps);
    return props;
  }

  public IsolateThread getPollingThread() {
      return pollingThread;
  }

  public void setPollingThread(IsolateThread pollingThread) {
      this.pollingThread = pollingThread;
  }

  public static Properties defaultProperties() {
    Properties props = new Properties();
    props.put("bootstrap.servers", "localhost:9092");
    props.put("enable.auto.commit", "false");
    props.put("auto.commit.interval.ms", "1000");
    props.put("key.deserializer", "org.apache.kafka.common.serialization.StringDeserializer");
    props.put("value.deserializer", "org.apache.kafka.common.serialization.StringDeserializer");
    props.put("auto.offset.reset", "earliest");
    props.put(
        ConsumerConfig.PARTITION_ASSIGNMENT_STRATEGY_CONFIG,
        org.apache.kafka.clients.consumer.RangeAssignor.class.getName());
    return props;
  }
}
