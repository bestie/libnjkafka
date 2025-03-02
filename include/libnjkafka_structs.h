#ifndef LIBNJKAFKA_STRUCTS_H
#define LIBNJKAFKA_STRUCTS_H

typedef struct {
    char* topic;
    int partition;
    long offset;
    char* metadata;
} libnjkafka_TopicPartitionOffsetAndMetadata;

typedef struct {
    int count;
    libnjkafka_TopicPartitionOffsetAndMetadata* items;
} libnjkafka_TopicPartitionOffsetAndMetadata_List;

typedef struct {
    char* topic;
    int partition;
} libnjkafka_TopicPartition;

typedef struct {
    int count;
    libnjkafka_TopicPartition* items;
} libnjkafka_TopicPartition_List;

typedef struct {
    long id;
} libnjkafka_Producer;

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
    int partition;
    long offset;
    char* key;
    char* topic;
    char* value;
} libnjkafka_ProducerRecord;

typedef struct {
    char* bootstrap_servers;
    char* client_id;
    int acks;
    int linger_ms;
    int max_in_flight_requests_per_connection;
    int retries;
    int batch_size;
    char* compression_type;
    int delivery_timeout_ms;
    int enable_idempotence;
    int max_request_size;
    int request_timeout_ms;
    int retry_backoff_ms;
    int metadata_max_age_ms;
    int message_timeout_ms;
} libnjkafka_ProducerConfig;

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

// This is not used in C code but is necessary to have a generic type for Java
typedef struct {
    int count;
    void* items;
} libnjkafka_ArrayWrapper;

#endif
