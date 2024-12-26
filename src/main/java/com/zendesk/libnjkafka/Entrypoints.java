package src.main.java.com.zendesk.libnjkafka;

import java.time.Duration;
import java.util.Properties;
// import java.util.Set;

import org.apache.kafka.clients.consumer.ConsumerRecord;
import org.apache.kafka.clients.consumer.ConsumerRecords;
// import org.apache.kafka.common.TopicPartition;
import org.graalvm.nativeimage.IsolateThread;
import org.graalvm.nativeimage.UnmanagedMemory;
import org.graalvm.nativeimage.c.function.CEntryPoint;
import org.graalvm.nativeimage.c.struct.SizeOf;
import org.graalvm.nativeimage.c.type.CCharPointer;
import org.graalvm.nativeimage.c.type.CTypeConversion;
import org.graalvm.word.Pointer;
import org.graalvm.word.UnsignedWord;

import src.main.java.com.zendesk.libnjkafka.ConsumerProxy;
import src.main.java.com.zendesk.libnjkafka.ConsumerRegistry;
import src.main.java.com.zendesk.libnjkafka.Structs.ConsumerRecordLayout;
import src.main.java.com.zendesk.libnjkafka.Structs.ConsumerRecordListLayout;
import src.main.java.com.zendesk.libnjkafka.Structs.ConsumerConfigLayout;

public class Entrypoints {
    public static ConsumerRegistry consumerRegistry = new ConsumerRegistry();

    @CEntryPoint(name = "libnjkafka_java_create_consumer")
    public static long createConsumer(IsolateThread thread, CCharPointer cGroupId) {
        try {
            String groupId = CTypeConversion.toJavaString(cGroupId);
            Properties config = new Properties();
            config.setProperty("group.id", groupId);

            ConsumerProxy consumer = ConsumerProxy.createConsumer(config);
            long consumerId = consumerRegistry.add(consumer);
            return consumerId;
        } catch (Exception e) {
            e.printStackTrace();
            return -1;
        }
    }

    @CEntryPoint(name = "libnjkafka_java_create_consumer_with_config")
    public static long createConsumer(IsolateThread thread, ConsumerConfigLayout cConfig) {
        try {
            Properties config = configToProps(cConfig);
            ConsumerProxy consumer = ConsumerProxy.createConsumer(config);
            long consumerId = consumerRegistry.add(consumer);
            return consumerId;
        } catch (Exception e) {
            e.printStackTrace();
            return -1;
        }
    }

    @CEntryPoint(name = "libnjkafka_java_consumer_subscribe")
    public static int subscribe(IsolateThread thread, long consumerId, CCharPointer topicC) {
        try {
            String topicName = CTypeConversion.toJavaString(topicC);
            ConsumerProxy consumer = consumerRegistry.get(consumerId);
            consumer.subscribe(topicName);
            return 0;
        } catch (Exception e) {
            e.printStackTrace();
            return -1;
        }
    }

    @CEntryPoint(name = "libnjkafka_java_consumer_commit_all_sync")
    public static int commitSync(IsolateThread thread, long consumerId, int timeout_milliseconds) {
        try {
            Duration timeout = Duration.ofMillis(timeout_milliseconds);
            ConsumerProxy consumer = consumerRegistry.get(consumerId);
            consumer.commitSync(timeout);
            return 0;
        } catch (Exception e) {
            e.printStackTrace();
            return -1;
        }
    }

    @CEntryPoint(name = "libnjkafka_java_consumer_poll")
    public static ConsumerRecordListLayout poll(IsolateThread thread, long consumerId, long duration) {
        ConsumerProxy consumer = consumerRegistry.get(consumerId);

        ConsumerRecords<String, String> records = consumer.poll(Duration.ofMillis(duration));

        UnsignedWord struct_size = SizeOf.unsigned(ConsumerRecordLayout.class);
        UnsignedWord totalMemorySize = struct_size.multiply(records.count());

        Pointer recordArray = UnmanagedMemory.calloc(totalMemorySize);

        int i = 0;
        for (ConsumerRecord<String, String> record : records) {
            UnsignedWord offset = struct_size.multiply(i);

            ConsumerRecordLayout cRecord = (ConsumerRecordLayout) recordArray.add(offset);

            cRecord.setOffset(record.offset());
            cRecord.setPartition(record.partition());
            cRecord.setTimestamp(record.timestamp());
            cRecord.setKey(CTypeConversion.toCString(record.key()).get());
            cRecord.setTopic(CTypeConversion.toCString(record.topic()).get());
            cRecord.setValue(CTypeConversion.toCString(record.value()).get());

            i++;
        }

        ConsumerRecordListLayout recordArrayWrapper = UnmanagedMemory
                .calloc(SizeOf.unsigned(ConsumerRecordListLayout.class));
        recordArrayWrapper.setCount(i);
        recordArrayWrapper.setRecords(recordArray);

        return recordArrayWrapper;
    }

    @CEntryPoint(name = "libnjkafka_java_consumer_close")
    public static int close(IsolateThread thread, long consumerId) {
        try {
            ConsumerProxy consumer = consumerRegistry.get(consumerId);
            consumer.close();
            consumerRegistry.remove(consumerId);
            return 0;
        } catch (Exception e) {
            return -1;
        }
    }

    public static Properties configToProps(ConsumerConfigLayout config) {
        Properties props = new Properties();

        setProperty(props, "bootstrap.servers", config.getBootstrapServers());
        setProperty(props, "group.id", config.getGroupId());
        setProperty(props, "enable.auto.commit", config.getEnableAutoCommit() == 1 ? "true" : "false");
        setProperty(props, "auto.commit.interval.ms", String.valueOf(config.getAutoCommitIntervalMs()));
        setProperty(props, "session.timeout.ms", String.valueOf(config.getSessionTimeoutMs()));
        setProperty(props, "request.timeout.ms", String.valueOf(config.getRequestTimeoutMs()));
        setProperty(props, "fetch.min.bytes", String.valueOf(config.getFetchMinBytes()));
        setProperty(props, "fetch.max.wait.ms", String.valueOf(config.getFetchMaxWaitMs()));
        setProperty(props, "max.partition.fetch.bytes", String.valueOf(config.getMaxPartitionFetchBytes()));
        setProperty(props, "isolation.level", config.getIsolationLevel());
        setProperty(props, "max.poll.records", String.valueOf(config.getMaxPollRecords()));
        setProperty(props, "max.poll.interval.ms", String.valueOf(config.getMaxPollIntervalMs()));
        setProperty(props, "auto.offset.reset", config.getAutoOffsetReset());
        setProperty(props, "client.id", config.getClientId());
        setProperty(props, "heartbeat.interval.ms", String.valueOf(config.getHeartbeatIntervalMs()));
        setProperty(props, "check.crcs", config.getCheckCrcs() == 1 ? "true" : "false");
        setProperty(props, "fetch.max.bytes", String.valueOf(config.getFetchMaxBytes()));

        return props;
    }

    private static void setProperty(Properties props, String key, CCharPointer cValue) {
        String javaValue = CTypeConversion.toJavaString(cValue);
        System.out.println("++++++++++++++++++ User config: Coverting " + key + " to `" + javaValue + "` from C String");
        setProperty(props, key, javaValue);
    }

    private static void setProperty(Properties props, String key, String value) {
        if (value != null && !value.trim().isEmpty()) {
            System.out.println("++++++++++++++++++ User config: Setting " + key + " to " + value+ " Java string");
            props.setProperty(key, value);
        } else {
            System.out.println("++++++++++++++++++ User config: Skipping " + key + " as it is empty");
        }
    }
}
