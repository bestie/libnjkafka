package com.zendesk.libnjkafka;

import java.time.Duration;
import java.util.Properties;
import java.util.Set;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.lang.Exception;

import org.apache.kafka.clients.consumer.ConsumerRebalanceListener;
import org.apache.kafka.clients.consumer.ConsumerRecord;
import org.apache.kafka.clients.consumer.ConsumerRecords;
import org.apache.kafka.clients.consumer.OffsetAndMetadata;
import org.apache.kafka.clients.producer.ProducerRecord;
import org.apache.kafka.common.TopicPartition;

public class JavaDemo {
    private static List<String> topics;

	public static void main(String[] args) {
        System.out.println("Starting Java consumer demo.");
        System.out.println("This is mainly used for generating a dependency config for the native image.");

        String topicName = System.getenv("KAFKA_TOPIC");

        ProducerProxy producer = producer();

        send_messages(producer, topicName);

        ConsumerProxy consumer = consumer();
        topics = List.of(topicName);

        consumer.subscribe(topics, new ConsumerRebalanceListener() {
            @Override
            public void onPartitionsRevoked(Collection<TopicPartition> partitions) {
                System.out.println(">>> 👂👂👂👂👂👂👂👂👂👂👂👂Partitions revoked: " + partitions);
            }

            @Override
            public void onPartitionsAssigned(Collection<TopicPartition> partitions) {
                System.out.println(">>> 👂👂👂👂👂👂👂👂👂👂👂👂Partitions assigned: " + partitions);
            }
        });

        processMessages(consumer);
        checkAssignedPartitions(consumer);
        checkCommittedOffsets(consumer);
        commitOffsets(consumer);
        checkCommittedOffsets(consumer);

        System.out.println("Done");
    }

    private static void send_messages(ProducerProxy producer, String topic) {
        for (int i = 0; i < 120; i++) {
		    String key = Integer.toString(i);
		    String value = "message-" + i;
		    int partition = i % 12;
		    ProducerRecord<String, String> record = new ProducerRecord<>(topic, partition, key, value);
		    System.out.println("Sending message: " + record);
		    producer.send(record);
		}
    }

    private static ProducerProxy producer() { 
        Properties props = new Properties();
        props.setProperty("bootstrap.servers", System.getenv("KAFKA_BROKERS"));
        ProducerProxy producer = ProducerProxy.create(props);
        return producer;
    }

    private static ConsumerProxy consumer() {
        long unixTime = System.currentTimeMillis() / 1000L;
        String groupId = "test-group-" + unixTime;
    
        Properties props = new Properties();
        props.setProperty("bootstrap.servers", System.getenv("KAFKA_BROKERS"));
        props.setProperty("group.id", groupId);

        ConsumerProxy unRegisteredConsumer = ConsumerProxy.create(props);
        long consumerId = Entrypoints.consumerRegistry.add(unRegisteredConsumer);
        ConsumerProxy consumer = Entrypoints.consumerRegistry.get(consumerId);
        return consumer;
    }

    private static void processMessages(ConsumerProxy consumer) {
        System.out.println("Polling for messages");
        ConsumerRecords<String, String> records = consumer.poll(Duration.ofMillis(1000));
        for (ConsumerRecord<String, String> record : records) {
            System.out.println("Received message: " + record.value());
        }
    }

    private static void checkAssignedPartitions(ConsumerProxy consumer) {
        Set<TopicPartition> assignedPartitions = consumer.assignment();
        System.out.println("Assigned partitions: " + assignedPartitions);
    }

    private static void commitOffsets(ConsumerProxy consumer) {
        System.out.println("Committing offsets");
        consumer.commitSync(Duration.ofMillis(1000));
    }

    private static void checkCommittedOffsets(ConsumerProxy consumer) {
        Set<TopicPartition> assignedPartitions = consumer.assignment();
        Map<TopicPartition, OffsetAndMetadata> committedOffsets = consumer.committed(assignedPartitions, Duration.ofMillis(1000));

        for (Map.Entry<TopicPartition, OffsetAndMetadata> entry : committedOffsets.entrySet()) {
            OffsetAndMetadata ofm = entry.getValue();
            String offset = ofm == null ? "null" : Long.toString(ofm.offset());
            System.out.println("Committed offset for partition " + entry.getKey() + " is " + offset);
        }
    }

    private static void closeConsumer(ConsumerProxy consumer) {
        try {
            System.out.println("Closing consumer");
            consumer.close();
        } catch (Exception e) {
            System.out.println("Error closing consumer: " + e);
        }
    }
}
