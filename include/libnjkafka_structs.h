#ifndef LIBNJKAFKA_STRUCTS_H
#define LIBNJKAFKA_STRUCTS_H

typedef struct {
    long id;
} libnjkafka_Consumer;

typedef struct {
    int partition;
    long offset;
    long timestamp;
    char* key;
    char* topic;
    char* value;
} libnjkafka_ConsumerRecord;

typedef struct {
    int count;
    libnjkafka_ConsumerRecord* records;
} libnjkafka_ConsumerRecord_List;

typedef struct {
    char* bootstrap_servers;
    char* group_id;
    int enable_auto_commit;
    int auto_commit_interval_ms;
    char* offset_reset;
    int session_timeout_ms;
    int request_timeout_ms;
    int fetch_min_bytes;
    int fetch_max_wait_ms;
    int max_partition_fetch_bytes;
    char* isolation_level;
    int max_poll_records;
    int max_poll_interval_ms;
    char* auto_offset_reset;
    char* client_id;
    int heartbeat_interval_ms;
    int check_crcs;
    int fetch_max_bytes;
} libnjkafka_ConsumerConfig;

typedef int (*libnjkafka_ConsumerRecord_Processor)(libnjkafka_ConsumerRecord record);

typedef struct {
    int total_records;
    int success_count;
} libnjkafka_BatchResults;

#endif
