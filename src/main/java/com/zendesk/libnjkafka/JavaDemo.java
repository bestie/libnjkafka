package com.zendesk.libnjkafka;

import java.time.Duration;
import java.util.Properties;
import java.util.Set;
import java.util.List;

import org.apache.kafka.clients.consumer.ConsumerRecord;
import org.apache.kafka.clients.consumer.ConsumerRecords;
import org.apache.kafka.common.TopicPartition;

public class JavaDemo {
    public static void main(String[] args) {
        System.out.println("Starting Java consumer demo.");
        System.out.println("This is mainly used for generating a dependency config for the native image.");

        String topicName = System.getenv("KAFKA_TOPIC");
        long unixTime = System.currentTimeMillis() / 1000L;
        String groupId = "test-group-" + unixTime;
    
        Properties props = new Properties();
        props.setProperty("bootstrap.servers", System.getenv("KAFKA_BROKERS"));
        props.setProperty("group.id", groupId);

        ConsumerProxy unRegisteredConsumer = ConsumerProxy.create(props);
        long consumerId = Entrypoints.consumerRegistry.add(unRegisteredConsumer);
        ConsumerProxy consumer = Entrypoints.consumerRegistry.get(consumerId);

        consumer.subscribe(List.of(topicName));

        int targetMessageCount = 120;
        int totalMessagesProcessed = 0;
        int i = 0;

        while (totalMessagesProcessed < targetMessageCount) {
            ConsumerRecords<String, String> records = consumer.poll(Duration.ofMillis(100));

            Set<TopicPartition> assignedPartitions = consumer.assignment();
            System.out.println("Poll i = " + i + ", assigned partitions:" + assignedPartitions);

            for (ConsumerRecord<String, String> record : records) {
                System.out.printf("offset = %d, key = %s, value = %s%n", record.offset(), record.key(), record.value());
                totalMessagesProcessed++;
                if (totalMessagesProcessed >= targetMessageCount) {
                    break;
                }
            }
        }

        System.out.printf("Processed message count = %d\n", totalMessagesProcessed);

        System.out.println("Committing offsets");
        consumer.commitSync(Duration.ofMillis(1000));

        Set<TopicPartition> assignedpartitions = consumer.assignment();
        System.out.println("assigned partitions:" + assignedpartitions);

        System.out.println("Closing consumer");
        consumer.close();

        System.out.println("Done");
    }
}
