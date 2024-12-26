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
    int auto_commit_interval_ms;
    char* auto_offset_reset;
    char* bootstrap_servers;
    int check_crcs;
    char* client_id;
    int enable_auto_commit;
    int fetch_max_bytes;
    int fetch_max_wait_ms;
    int fetch_min_bytes;
    char* group_id;
    int heartbeat_interval_ms;
    char* isolation_level;
    int max_partition_fetch_bytes;
    int max_poll_interval_ms;
    int max_poll_records;
    char* offset_reset;
    int request_timeout_ms;
    int session_timeout_ms;
} libnjkafka_ConsumerConfig;

typedef struct {
    int total_records;
    int success_count;
} libnjkafka_BatchResults;

#endif
