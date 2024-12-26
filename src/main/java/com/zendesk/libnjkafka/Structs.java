package src.main.java.com.zendesk.libnjkafka;

import java.util.Collections;
import java.util.List;

import org.graalvm.nativeimage.c.CContext;
import org.graalvm.nativeimage.c.struct.CField;
import org.graalvm.nativeimage.c.struct.CStruct;
import org.graalvm.nativeimage.c.type.CCharPointer;
import org.graalvm.word.PointerBase;

@CContext(Structs.Directives.class)
public class Structs {
    static class Directives implements CContext.Directives {
        @Override
        public List<String> getHeaderFiles() {
            return Collections.singletonList("\"libnjkafka_structs.h\"");
        }
    }

    @CStruct("libnjkafka_ConsumerRecord")
    interface ConsumerRecordLayout extends PointerBase {
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
    interface ConsumerRecordListLayout extends PointerBase {
        @CField("count")
        int getCount();
        @CField("count")
        void setCount(int count);

        @CField("records")
        PointerBase getRecords();
        @CField("records")
        void setRecords(PointerBase records);
    }

    @CStruct("libnjkafka_ConsumerConfig")
    public interface ConsumerConfigLayout extends PointerBase {
        @CField("auto_commit_interval_ms")
        int getAutoCommitIntervalMs();

        @CField("auto_commit_interval_ms")
        void setAutoCommitIntervalMs(int auto_commit_interval_ms);

        @CField("auto_offset_reset")
        void setAutoOffsetReset(CCharPointer auto_offset_reset);

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
}
