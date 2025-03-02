#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "libnjkafka.h"
#include "libnjkafka_callbacks.h"

#define MESSAGE_COUNT_ENV_KEY "EXECPTED_MESSAGE_COUNT"

#define DEFAULT_PARTITIONS 12
#define DEFAULT_EXPECTED_MESSAGE_COUNT 120
#define DEFAULT_MESSAGES_PER_PARTITION 10

#define RED   "\x1B[31m"
#define GREEN "\x1B[32m"
#define RESET "\x1B[0m"

libnjkafka_Producer* producer;

int print_message(libnjkafka_ConsumerRecord record, void* opaque) {
    printf("Message partition %d, offset %ld, key %s, value `%s`\n", record.partition, record.offset, record.key, record.value);
    return 0;
}

int assigned_partitions[DEFAULT_PARTITIONS] = {};
int revoked_partitions[DEFAULT_PARTITIONS] = {};
bool rebalance_listener_called = false;

void print_partitions(char event[8], libnjkafka_TopicPartition_List* topic_partitions) {
    rebalance_listener_called = true;
    printf("👂Rebalance callback: partitions %s: %d\n", event, topic_partitions->count);
    for(int i=0; i<topic_partitions->count; i++) {
        libnjkafka_TopicPartition tp = topic_partitions->items[i];
        printf(" 👂👂👂 assigned Partition: %d\n", tp.partition);
    }
}
void partitions_assigned(void* gvm_thread, libnjkafka_TopicPartition_List* topic_partitions) {
    printf(" CCCCCCCCCCCCCCCC   Assigned partitions: %d\n", topic_partitions->count);
    char event[] = "assigned";
    print_partitions(event, topic_partitions);
    for(int i=0; i<topic_partitions->count; i++) {
      assigned_partitions[topic_partitions->items[i].partition] = 1;
    }
}
void partitions_revoked(void* gvm_thread, libnjkafka_TopicPartition_List* topic_partitions) {
    printf("👂Revoked partitions: %d\n", topic_partitions->count);
    char event[] = "revoked\0";
    print_partitions(event, topic_partitions);
    for(int i=0; i<topic_partitions->count; i++) {
      revoked_partitions[topic_partitions->items[i].partition] = 1;
    }
}

void partitions_lost(void* gvm_thread, libnjkafka_TopicPartition_List* topic_partitions) {
    printf("👂Lost partitions: %d\n", topic_partitions->count);
    char event[] = "lost\0";
    print_partitions(event, topic_partitions);
}

bool ensure_partitions_assigned_callback_called() {
    bool partitions_assigned_callback_success = true;
    for(int i=0; i<DEFAULT_PARTITIONS; i++) {
      if(assigned_partitions[i] != 1) {
        printf(RED "libnjkafka_consumer_rebalance_listener Error: Expected partition %d to be assigned\n" RESET, i);
        partitions_assigned_callback_success = false;
      }
    }
    if(partitions_assigned_callback_success) {
      printf(GREEN "libnjkafka_consumer_rebalance_listener Success: All partitions were assigned\n" RESET);
    }
    return partitions_assigned_callback_success;
}

bool ensure_partitions_revoked_callback_called() {
    bool partitions_revoked_callback_success = true;
    for(int i=0; i<DEFAULT_PARTITIONS; i++) {
      if(revoked_partitions[i] != 1) {
        printf(RED "libnjkafka_consumer_rebalance_listener Error: Expected partition %d to be revoked\n" RESET, i);
        partitions_revoked_callback_success = false;
      }
    }
    if(partitions_revoked_callback_success) {
      printf(GREEN "libnjkafka_consumer_rebalance_listener Success: All partitions were revoked\n" RESET);
    }
    return partitions_revoked_callback_success;
}

int thread_count = 0;

// run in a separate thread
void consumer_poll(libnjkafka_Consumer* consumer) {
    libnjkafka_init_thread();
    int thread_n = thread_count++;

    for(int i=0; i<20; i++) {
      printf(" 🧵 thread-%d Polling for records in thread\n", thread_n);
      libnjkafka_ConsumerRecord_List* record_list = libnjkafka_consumer_poll(consumer, 1000);
      printf(" 🧵 thread-%d Got %d records\n", thread_n, record_list->count);
      free(record_list);

      libnjkafka_TopicPartition_List* topic_partitions = libnjkafka_consumer_assignment(consumer);
      printf(" 🧵 thread-%d Consumer#assigned partitions: %d\n", thread_n, topic_partitions->count);
      free(topic_partitions);
      libnjkafka_consumer_commit_all_sync(consumer, 1000);
      usleep(500000);
    }

    printf(" 🧵 thread-%d Closing consumer\n", thread_n);
    libnjkafka_consumer_close(consumer);
    printf(" 🧵 thread-%d  Tearing down thread\n", thread_n);
    libnjkafka_teardown_thread();
}

libnjkafka_ProducerConfig* create_producer_config() {
    libnjkafka_ProducerConfig* config = (libnjkafka_ProducerConfig*)malloc(sizeof(libnjkafka_ProducerConfig));
    config->bootstrap_servers = strdup("localhost:9092");
    config->client_id = strdup("libnjkafka-c-test-producer");
    config->acks = 1;
    config->linger_ms = 0;
    config->max_in_flight_requests_per_connection = 5;
    config->retries = 0;
    config->batch_size = 16384;
    config->compression_type = strdup("none");
    config->delivery_timeout_ms = 30000;
    config->enable_idempotence = 0;
    config->max_request_size = 1048576;
    config->request_timeout_ms = 30000;
    config->retry_backoff_ms = 100;
    config->metadata_max_age_ms = 300000;
    config->message_timeout_ms = 300000;
    return config;
}

void produce_message(libnjkafka_Producer* producer, char* topic, char* key, char* message, int partition) {
    libnjkafka_ProducerRecord* record = (libnjkafka_ProducerRecord*)malloc(sizeof(libnjkafka_ProducerRecord));
    record->topic = topic;
    record->key = key;
    record->value = message;
    record->partition = partition;
    free(record);
}

void produce_messages(libnjkafka_Producer* producer, char* topic, int partitions, int total_messages) {
    for(int i = 0; i < total_messages; i++) {
        int partition = i % partitions;
        char message[100];
        snprintf(message, 100, "Message %d published to partition %d\n", i, partition);
        produce_message(producer, topic, "", message, partition);
    }
    free(topic);
}

int main() {
    srand(time(NULL));
    libnjkafka_init();

    char* group_id = (char*)malloc(30);
    snprintf(group_id, 30, "test-group-%d", rand());

    const char* kafka_topic = getenv("KAFKA_TOPIC");
    if(!kafka_topic) {
      printf("KAFKA_TOPIC env variable not set");
      exit(1);
    }

    const char* kafka_brokers = getenv("KAFKA_BROKERS");
    if(!kafka_brokers) {
      printf("KAFKA_BROKERS env variable not set");
      exit(1);
    }

    libnjkafka_ProducerConfig* producer_config = create_producer_config();
    producer_config->bootstrap_servers = strdup(kafka_brokers);
    producer_config->client_id = strdup("libnjkafka-c-test-producer");
    producer = libnjkafka_create_producer(producer_config);
    produce_messages(producer, strdup(kafka_topic), DEFAULT_PARTITIONS, DEFAULT_EXPECTED_MESSAGE_COUNT);
    libnjkafka_producer_close(producer);

    libnjkafka_ConsumerConfig* config = (libnjkafka_ConsumerConfig*)malloc(sizeof(libnjkafka_ConsumerConfig));
    config->auto_commit_interval_ms = 5000;
    config->auto_offset_reset = strdup("earliest");
    config->bootstrap_servers = strdup(kafka_brokers);
    config->check_crcs = 1;
    config->client_id = strdup("libnjkafka-c-test");
    config->enable_auto_commit = 0;
    config->fetch_max_bytes = 52428800;
    config->fetch_max_wait_ms = 500;
    config->fetch_min_bytes = 1;
    config->group_id = strdup(group_id);
    config->heartbeat_interval_ms = 3000;
    config->isolation_level = strdup("read_committed");
    config->max_partition_fetch_bytes = 1048576;
    config->max_poll_interval_ms = 300000;
    config->max_poll_records = 500;
    config->offset_reset = strdup("earliest");
    config->request_timeout_ms = 30000;
    config->session_timeout_ms = 10000;

    libnjkafka_Consumer* consumer = libnjkafka_create_consumer(config);

    libnjkafka_ConsumerRebalanceListener* rebalance_listener = malloc(sizeof(libnjkafka_ConsumerRebalanceListener));
    rebalance_listener->on_partitions_assigned = (libnjkafka_ConsumerRebalanceCallback)&partitions_assigned;
    rebalance_listener->on_partitions_revoked = (libnjkafka_ConsumerRebalanceCallback)&partitions_revoked;
    rebalance_listener->on_partitions_lost = (libnjkafka_ConsumerRebalanceCallback)&partitions_lost;

    libnjkafka_consumer_subscribe(consumer, strdup(kafka_topic), rebalance_listener);

    int processed_messages = 0;
    int max_attempts = 5;
    int attempts = 0;

    libnjkafka_TopicPartition_List* topic_partitions;
    topic_partitions = libnjkafka_consumer_assignment(consumer);
    printf("  Consumer#assigned partitions: %d\n", topic_partitions->count);
    free(topic_partitions);

    while(processed_messages < DEFAULT_EXPECTED_MESSAGE_COUNT && attempts < max_attempts) {
      attempts++;
      libnjkafka_ConsumerRecord_List* record_list = libnjkafka_consumer_poll(consumer, 1000);
      printf("Polled - message count: %d\n", record_list->count);

      if (record_list == NULL) {
          printf("Error polling for messages\n");
          break;
      } else {
          printf("Sucessfully polled. %d\n", record_list->count);
      }

      for(int i = 0; i < record_list->count; i++) {
          libnjkafka_ConsumerRecord record = record_list->records[i];
          printf(" processed: partition %d, offset %ld, key %s, value `%s`\n", record.partition, record.offset, record.key, record.value);
          processed_messages++;
      }

      free(record_list);
    }

    topic_partitions = libnjkafka_consumer_assignment(consumer);
    printf("Assigned partitions: %d\n", topic_partitions->count);

    char* assigned_topic = topic_partitions->items[0].topic;
    if(strcmp(assigned_topic, kafka_topic) == 0) {
      printf(GREEN "libnjkafka_consumer_assignment return topic: %s\n" RESET, assigned_topic);
    } else {
      printf(RED "libnjkafka_consumer_assignment Error: Expected topic %s, got %s\n" RESET, kafka_topic, assigned_topic);
      exit(1);
    }

    if(topic_partitions->count == DEFAULT_PARTITIONS) {
      printf(GREEN "libnjkafka_consumer_assignment returned %d assiged partitions:\n" RESET, topic_partitions->count);

      for(int i=0; i<topic_partitions->count; i++) {
        libnjkafka_TopicPartition tp = topic_partitions->items[i];
        printf("  assigned Partition: %d\n", tp.partition);
      }
    } else {
      printf(RED "libnjkafka_consumer_assignment Error: Expected %d, got %d\n" RESET, DEFAULT_PARTITIONS, topic_partitions->count);
      exit(1);
    }

    if(processed_messages == DEFAULT_EXPECTED_MESSAGE_COUNT) {
      printf(GREEN "libnjkafka_consumer_poll Processed %d messages as expected.\n" RESET, processed_messages);
    } else {
      printf(RED "libnjkafka_consumer_poll Error: Expected %d, got %d\n" RESET, DEFAULT_EXPECTED_MESSAGE_COUNT, processed_messages);
      exit(1);
    }

    libnjkafka_consumer_commit_all_sync(consumer, 1000);

    libnjkafka_TopicPartitionOffsetAndMetadata_List* offsets = libnjkafka_consumer_committed(consumer, topic_partitions, 1000);
    free(topic_partitions);

    if(offsets->count == DEFAULT_PARTITIONS) {
      printf(GREEN "libnjkafka_consumer_assignment returned %d assiged partitions:\n" RESET, offsets->count);

      for(int i = 0; i < offsets->count; i++) {
          libnjkafka_TopicPartitionOffsetAndMetadata offset = offsets->items[i];

          if(strcmp(offset.topic, kafka_topic) != 0) {
            printf(RED "libnjkafka_consumer_committed Error: Expected topic %s, got %s\n" RESET, kafka_topic, offset.topic);
            exit(1);
          }

          if(offset.offset == DEFAULT_MESSAGES_PER_PARTITION) {
              printf(GREEN "  committed offset for topic %s partition %d: %ld\n" RESET, offset.topic, offset.partition, offset.offset);
          } else {
              printf(RED "  committed offset for topic %s partition %d: %ld\n" RESET, offset.topic, offset.partition, offset.offset);
              exit(1);
          }
      }
    } else {
      printf(RED "libnjkafka_consumer_assignment Error: Expected %d, got %d\n" RESET, DEFAULT_PARTITIONS, offsets->count);
      exit(1);
    }

    free(offsets);

    libnjkafka_consumer_close(consumer);
    ensure_partitions_revoked_callback_called();

    printf("\n\n");
    printf(GREEN "libnjkafka_consumer_poll Processed %d messages as expected.\n" RESET, DEFAULT_EXPECTED_MESSAGE_COUNT);
    printf("\n\n");

    printf("Now try processing with a callback!!!!!!!!!!!!!!!!!!!!!!!!!! 🤙 🤙 🤙\n\n");
    char* group_id2 = (char*)malloc(30);
    snprintf(group_id2, 30, "test-group-%d", rand());

    config->group_id = strdup(group_id2);
    libnjkafka_Consumer* consumer2 = libnjkafka_create_consumer(config);
    libnjkafka_consumer_subscribe(consumer2, strdup(kafka_topic), rebalance_listener);

    void* opaque = NULL;
    libnjkafka_ConsumerRecordProcessor* processor = (libnjkafka_ConsumerRecordProcessor*)print_message;
    libnjkafka_BatchResults results = libnjkafka_consumer_poll_each_message(consumer2, 100, processor, opaque);

    if(results.success_count != DEFAULT_EXPECTED_MESSAGE_COUNT) {
      printf(RED "libnjkafka_consumer_poll_each_message Error: Expected %d, got %d\n" RESET, DEFAULT_EXPECTED_MESSAGE_COUNT, results.success_count);
      exit(1);
    }

    printf("\n\n");
    printf(GREEN "libnjkafka_consumer_poll_each_message Processed %d messages as expected.\n" RESET, DEFAULT_EXPECTED_MESSAGE_COUNT);
    printf("\n\n");

    libnjkafka_consumer_close(consumer2);

    printf("\n\n");
    printf("Test the rebalance listener 👂👂\n");
    printf("\n\n");

    char* group_id_pt = (char*)malloc(30);
    snprintf(group_id_pt, 30, "test-group-%d", rand());
    config->group_id = strdup(group_id_pt);
    config->max_poll_records = 1;

    libnjkafka_Consumer* consumer_t1 = libnjkafka_create_consumer(config);
    libnjkafka_consumer_subscribe(consumer_t1, strdup(kafka_topic), rebalance_listener);
    libnjkafka_Consumer* consumer_t2 = libnjkafka_create_consumer(config);
    libnjkafka_consumer_subscribe(consumer_t2, strdup(kafka_topic), rebalance_listener);

    pthread_t pt1;
    pthread_t pt2;
    pthread_create(&pt1, NULL, (void*)consumer_poll, consumer_t1);
    sleep(4);
    pthread_create(&pt2, NULL, (void*)consumer_poll, consumer_t2);

    printf("Waiting for rebalance listener to be called\n");
    pthread_join(pt1, NULL);
    printf("Thread 1 joined\n");
    pthread_join(pt2, NULL);
    printf("Thread 2 joined\n");

    if(rebalance_listener_called) {
      printf(GREEN "libnjkafka_consumer_subscribe Success: Rebalance listener was called\n" RESET);
    } else {
      printf(RED "libnjkafka_consumer_subscribe Error: Rebalance listener was not called\n" RESET);
      exit(1);
    }

    ensure_partitions_assigned_callback_called();

    libnjkafka_teardown();
    return 0;
}
