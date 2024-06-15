package src.main.java.com.zendesk.libnjkafka;

import org.apache.kafka.clients.consumer.ConsumerRecords;
import org.apache.kafka.clients.consumer.KafkaConsumer;

import java.time.Duration;
import java.util.Arrays;
import java.util.Properties;

public class ConsumerProxy {
    private KafkaConsumer<String, String> consumer;

    public static ConsumerProxy createConsumer(Properties configuredProperties) {
        Properties allConfig = mergeDefaultConfig(configuredProperties);

        KafkaConsumer<String, String> consumer = new KafkaConsumer<String, String>(allConfig);

        ConsumerProxy instance = new ConsumerProxy(consumer);
        return instance;
    }

    public ConsumerProxy(KafkaConsumer<String, String> consumer) {
        this.consumer = consumer;
    }

    public void subscribe(String topicName) {
        consumer.subscribe(Arrays.asList(topicName));
    }

    public ConsumerRecords<String, String> poll(Duration duration) {
        return consumer.poll(duration);
    }

    public void commitSync(Duration timeout) {
        consumer.commitSync(timeout);
    }

    public void close() {
        consumer.close();
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
