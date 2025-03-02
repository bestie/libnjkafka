package com.zendesk.libnjkafka;

import java.lang.String;
import java.util.Properties;

import org.apache.kafka.clients.producer.KafkaProducer;
import org.apache.kafka.clients.producer.ProducerRecord;
import org.apache.kafka.clients.producer.RecordMetadata;

import java.util.concurrent.Future;


public class ProducerProxy {
    private final KafkaProducer<String, String> delegate;

    public static ProducerProxy create(Properties properties) {
        return new ProducerProxy(new KafkaProducer<>(mergeDefaultConfig(properties)));
    }

    public ProducerProxy(KafkaProducer<String, String> delegate) {
        this.delegate = delegate;
    }

    public Future<RecordMetadata> send(ProducerRecord<String, String> record) {
        System.out.println("ProducerProxy: message=" + record.value() + ",partition=" + record.partition());
        Future<RecordMetadata> future = delegate.send(record, (metadata, exception) -> {
            //if (exception != null) {
            //    System.err.println("Error sending message: " + exception.getMessage());
            //} else {
            //    System.out.println("Message sent to partition " + metadata.partition() + " with offset " + metadata.offset());
            //}
        });
		return future;
    }

    public void close() {
        delegate.close();
    }

    private static Properties mergeDefaultConfig(Properties configuredProps) {
        Properties props = defaultProperties();
        props.putAll(configuredProps);
        return props;
    }

    public static Properties defaultProperties() {
        Properties props = new Properties();
        props.put("bootstrap.servers", "localhost:9092");
        props.put("key.serializer", "org.apache.kafka.common.serialization.StringSerializer");
        props.put("value.serializer", "org.apache.kafka.common.serialization.StringSerializer");
        props.put("acks", "1");
        props.put("linger.ms", "1");
        props.put("buffer.memory", "67108864");
        props.put("batch.size", "32768");
        //props.put("max.block.ms", "1000");
        //props.put("log.level", "DEBUG");

        return props;
    }
}
