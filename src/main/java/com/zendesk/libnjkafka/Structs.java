package com.zendesk.libnjkafka;

import java.util.List;
import java.util.HashSet;
import java.util.Set;

import org.graalvm.nativeimage.IsolateThread;
import org.graalvm.nativeimage.c.CContext;
import org.graalvm.nativeimage.c.struct.CField;
import org.graalvm.nativeimage.c.struct.CStruct;
import org.graalvm.nativeimage.c.type.CCharPointer;
import org.graalvm.word.Pointer;
import org.graalvm.word.PointerBase;
import org.graalvm.nativeimage.c.struct.SizeOf;
import org.graalvm.nativeimage.c.type.CTypeConversion;
import org.graalvm.nativeimage.c.function.CFunctionPointer;
import org.graalvm.nativeimage.c.function.InvokeCFunctionPointer;
import org.graalvm.word.UnsignedWord;

import org.apache.kafka.common.TopicPartition;

@CContext(Structs.Directives.class)
public class Structs {
    static class Directives implements CContext.Directives {
        @Override
        public List<String> getHeaderFiles() {
            return List.of(
                    "\"libnjkafka_structs.h\"",
                    "\"libnjkafka_callbacks.h\""
                    );

        }
    }

    @CStruct("libnjkafka_ProducerRecord")
    public interface ProducerRecordLayout extends PointerBase {

        @CField("partition")
        int getPartition();
        @CField("partition")
        void setPartition(int value);

        @CField("offset")
        long getOffset();
        @CField("offset")
        void setOffset(long value);

        @CField("key")
        CCharPointer getKey();
        @CField("key")
        void setKey(CCharPointer key);

        @CField("topic")
        CCharPointer getTopic();
        @CField("topic")
        void setTopic(CCharPointer topic);

        @CField("value")
        CCharPointer getValue();
        @CField("value")
        void setValue(CCharPointer value);
    }

    public interface RebalanceCallback extends CFunctionPointer {
        @InvokeCFunctionPointer
        void invoke(IsolateThread thread, TopicPartitionListLayout partitions);
    }

    @CStruct("libnjkafka_ConsumerRebalanceListener")
    public interface RebalanceListenerStruct extends PointerBase {
        @CField("on_partitions_assigned")
        RebalanceCallback onPartitionsAssigned();
        @CField("on_partitions_assigned")
        void onPartitionsAssigned(RebalanceCallback callback);

        @CField("on_partitions_revoked")
        RebalanceCallback onPartitionsRevoked();
        @CField("on_partitions_revoked")
        void onPartitionsRevoked(RebalanceCallback callback);

        @CField("on_partitions_lost")
        RebalanceCallback onPartitionsLost();
        @CField("on_partitions_lost")
        void onPartitionsLost(RebalanceCallback callback);
    }

    @CStruct("libnjkafka_ArrayWrapper")
    public interface ArrayWrapper extends PointerBase {
      @CField("count")
      int getCount();
      @CField("count")
      void setCount(int count);

      @CField("items")
      PointerBase getItems();
      @CField("items")
      void setItems(PointerBase items);
    }

    @CStruct("libnjkafka_TopicPartitionOffsetAndMetadata")
    public interface TopicPartitionOffsetAndMetadataLayout extends PointerBase {
        @CField("topic")
        CCharPointer getTopic();
        @CField("topic")
        void setTopic(CCharPointer topic);

        @CField("partition")
        int getPartition();
        @CField("partition")
        void setPartition(int partition);

        @CField("offset")
        long getOffset();
        @CField("offset")
        void setOffset(long offset);

        @CField("metadata")
        CCharPointer getMetadata();
        @CField("metadata")
        void setMetadata(CCharPointer metadata);
    }

    @CStruct("libnjkafka_TopicPartitionOffsetAndMetadata_List")
    public interface TopicPartitionOffsetAndMetadataListLayout extends PointerBase {
        @CField("count")
        int getCount();
        @CField("count")
        void setCount(int count);

        @CField("items")
        PointerBase getItems();
        @CField("items")
        void setItems(PointerBase items);
    }

    @CStruct("libnjkafka_TopicPartition")
    public interface TopicPartitionLayout extends PointerBase {
        @CField("topic")
        CCharPointer getTopic();
        @CField("topic")
        void setTopic(CCharPointer topic);

        @CField("partition")
        int getPartition();
        @CField("partition")
        void setPartition(int partition);
    }

    @CStruct("libnjkafka_TopicPartition_List")
    public interface TopicPartitionListLayout extends PointerBase {
        @CField("count")
        int getCount();
        @CField("count")
        void setCount(int count);

        @CField("items")
        PointerBase getItems();
        @CField("items")
        void setItems(PointerBase items);
    }

    @CStruct("libnjkafka_ConsumerRecord")
    public interface ConsumerRecordLayout extends PointerBase {
        @CField("partition")
        int getPartition();
        @CField("partition")
        void setPartition(int partition);

        @CField("offset")
        long getOffset();
        @CField("offset")
        void setOffset(long offset);

        @CField("timestamp")
        long getTimestamp();
        @CField("timestamp")
        void setTimestamp(long timestamp);

        @CField("key")
        CCharPointer getKey();
        @CField("key")
        void setKey(CCharPointer key);

        @CField("topic")
        CCharPointer getTopic();
        @CField("topic")
        void setTopic(CCharPointer topic);

        @CField("value")
        CCharPointer getValue();
        @CField("value")
        void setValue(CCharPointer value);
    }

    @CStruct("libnjkafka_ConsumerRecord_List")
    public interface ConsumerRecordListLayout extends PointerBase {
        @CField("count")
        int getCount();
        @CField("count")
        void setCount(int count);

        @CField("records")
        PointerBase getRecords();
        @CField("records")
        void setRecords(PointerBase records);
    }

    @CStruct("libnjkafka_ProducerConfig")
    public interface ProducerConfigLayout extends PointerBase {

        @CField("bootstrap_servers")
        CCharPointer getBootstrapServers();
        @CField("bootstrap_servers")
        void setBootstrapServers(CCharPointer bootstrap_servers);

        @CField("client_id")
        void setClientId(CCharPointer client_id);
        @CField("client_id")
        CCharPointer getClientId();

        @CField("acks")
        int getAcks();
        @CField("acks")
        void setAcks(int value);

        @CField("linger_ms")
        int getLingerMs();
        @CField("linger_ms")
        void setLinger_ms(int value);

        @CField("max_in_flight_requests_per_connection")
        int getMaxInFlightRequestsPerConnection();
        @CField("max_in_flight_requests_per_connection")
        void setMaxInFlightRequestsPerConnection(int value);

        @CField("retries")
        int getRetries();
        @CField("retries")
        void setRetries(int value);

        @CField("batch_size")
        int getBatchSize();
        @CField("batch_size")
        void setBatchSize(int value);

        @CField("compression_type")
        CCharPointer getCompressionType();
        @CField("compression_type")
        void setCompressionType(CCharPointer value);

        @CField("delivery_timeout_ms")
        int getDeliveryTimeoutMs();
        @CField("delivery_timeout_ms")
        void setDeliveryTimeoutMs(int value);

        @CField("enable_idempotence")
        int getEnableIdempotence();
        @CField("enable_idempotence")
        void setEnableIdempotence(int value);

        @CField("max_request_size")
        int getMaxRequestSize();
        @CField("max_request_size")
        void setMaxRequestSize(int value);

        @CField("request_timeout_ms")
        int getRequestTimeoutMs();
        @CField("request_timeout_ms")
        void setRequestTimeoutMs(int value);

        @CField("retry_backoff_ms")
        int getRetryBackoffMs();
        @CField("retry_backoff_ms")
        void setRetryBackoffMs(int value);

        @CField("metadata_max_age_ms")
        int getMetadataMaxAgeMs();
        @CField("metadata_max_age_ms")
        void setMetadataMaxAgeMs(int value);

        @CField("message_timeout_ms")
        int getMessageTimeoutMs();
        @CField("message_timeout_ms")
        void setMessageTimeoutMs(int value);
    }


    @CStruct("libnjkafka_ConsumerConfig")
    public interface ConsumerConfigLayout extends PointerBase {
        @CField("auto_commit_interval_ms")
        int getAutoCommitIntervalMs();

        @CField("auto_commit_interval_ms")
        void setAutoCommitIntervalMs(int auto_commit_interval_ms);

        @CField("auto_offset_reset")
        void setAutoOffsetReset(PointerBase auto_offset_reset);

        @CField("auto_offset_reset")
        CCharPointer getAutoOffsetReset();

        @CField("bootstrap_servers")
        CCharPointer getBootstrapServers();

        @CField("bootstrap_servers")
        void setBootstrapServers(CCharPointer bootstrap_servers);

        @CField("check_crcs")
        void setCheckCrcs(int check_crcs);

        @CField("check_crcs")
        int getCheckCrcs();

        @CField("client_id")
        void setClientId(CCharPointer client_id);

        @CField("client_id")
        CCharPointer getClientId();

        @CField("enable_auto_commit")
        int getEnableAutoCommit();

        @CField("enable_auto_commit")
        void setEnableAutoCommit(int enable_auto_commit);

        @CField("fetch_max_bytes")
        void setFetchMaxBytes(int fetch_max_bytes);

        @CField("fetch_max_bytes")
        int getFetchMaxBytes();

        @CField("fetch_max_wait_ms")
        int getFetchMaxWaitMs();

        @CField("fetch_max_wait_ms")
        void setFetchMaxWaitMs(int fetch_max_wait_ms);

        @CField("fetch_min_bytes")
        int getFetchMinBytes();

        @CField("fetch_min_bytes")
        void setFetchMinBytes(int fetch_min_bytes);

        @CField("group_id")
        void setGroupId(CCharPointer group_id);

        @CField("group_id")
        CCharPointer getGroupId();

        @CField("heartbeat_interval_ms")
        void setHeartbeatIntervalMs(int heartbeat_interval_ms);

        @CField("heartbeat_interval_ms")
        int getHeartbeatIntervalMs();

        @CField("isolation_level")
        void setIsolationLevel(CCharPointer isolation_level);

        @CField("isolation_level")
        CCharPointer getIsolationLevel();

        @CField("max_partition_fetch_bytes")
        void setMaxPartitionFetchBytes(int max_partition_fetch_bytes);

        @CField("max_partition_fetch_bytes")
        int getMaxPartitionFetchBytes();

        @CField("max_poll_interval_ms")
        void setMaxPollIntervalMs(int max_poll_interval_ms);

        @CField("max_poll_interval_ms")
        int getMaxPollIntervalMs();

        @CField("max_poll_records")
        void setMaxPollRecords(int max_poll_records);

        @CField("max_poll_records")
        int getMaxPollRecords();

        @CField("offset_reset")
        CCharPointer getOffsetReset();

        @CField("offset_reset")
        void setOffsetReset(CCharPointer offset_reset);

        @CField("request_timeout_ms")
        void setRequestTimeoutMs(int request_timeout_ms);

        @CField("request_timeout_ms")
        int getRequestTimeoutMs();

        @CField("session_timeout_ms")
        void setSessionTimeoutMs(int session_timeout_ms);

        @CField("session_timeout_ms")
        int getSessionTimeoutMs();
    }

    public static Set<TopicPartition> toJava(TopicPartitionListLayout cList) {
        HashSet<TopicPartition> javaSet = new HashSet<>();

        UnsignedWord structSize = SizeOf.unsigned(TopicPartitionLayout.class);
        Pointer listPointer = (Pointer) cList.getItems();

        for (int i = 0; i < cList.getCount(); i++) {
            UnsignedWord offset = structSize.multiply(i);
            TopicPartitionLayout cTopicPartition = (TopicPartitionLayout) listPointer.add(offset);

            String topic = CTypeConversion.toJavaString(cTopicPartition.getTopic());
            Integer partition = cTopicPartition.getPartition();

            TopicPartition javaTopicPartition = new TopicPartition(topic, partition);
            javaSet.add(javaTopicPartition);
        }

        return javaSet;
    }
}
