#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "libnjkafka.h"
#include "libnjkafka_callbacks.h"

#define MESSAGE_COUNT_ENV_KEY "EXECPTED_MESSAGE_COUNT"

#define DEFAULT_PARTITIONS 12
#define DEFAULT_EXPECTED_MESSAGE_COUNT 120
#define DEFAULT_MESSAGES_PER_PARTITIONS 10

#define RED   "\x1B[31m"
#define GREEN "\x1B[32m"
#define RESET "\x1B[0m"

int print_message(libnjkafka_ConsumerRecord record, void* opaque) {
    printf("Message partition %d, offset %ld, key %s, value `%s`\n", record.partition, record.offset, record.key, record.value);
    return 0;
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

    libnjkafka_ConsumerConfig* config = (libnjkafka_ConsumerConfig*)malloc(sizeof(libnjkafka_ConsumerConfig));
    config->auto_commit_interval_ms = 5000;
    config->auto_offset_reset = strdup("earliest");
    config->bootstrap_servers = strdup("localhost:9092");
    config->check_crcs = 1;
    config->client_id = strdup("my-client");
    config->enable_auto_commit = 1;
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

    libnjkafka_Consumer* consumer = libnjkafka_create_consumer_with_config(config);

    libnjkafka_consumer_subscribe(consumer, strdup(kafka_topic));

    int processed_messages = 0;
    int max_attempts = 3;
    int attempts = 0;

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
          printf("Message partition %d, offset %ld, key %s, value `%s`\n", record.partition, record.offset, record.key, record.value);
          processed_messages++;
      }

      free(record_list);
    }
    libnjkafka_consumer_commit_all_sync(consumer, 1000);

    if(processed_messages != DEFAULT_EXPECTED_MESSAGE_COUNT) {
      printf(RED "libnjkafka_consumer_poll Error: Expected %d, got %d\n" RESET, DEFAULT_EXPECTED_MESSAGE_COUNT, processed_messages);
      exit(1);
    }

    printf("\n\n");
    printf(GREEN "libnjkafka_consumer_poll Processed %d messages as expected.\n" RESET, DEFAULT_EXPECTED_MESSAGE_COUNT);
    printf("\n\n");

    printf("Now try processing with a callback!!!!!!!!!!!!!!!!!!!!!!!!!! 🤙 🤙 🤙\n\n");
    char* group_id2 = (char*)malloc(30);
    snprintf(group_id2, 30, "test-group-%d", rand());

    libnjkafka_Consumer* consumer2 = libnjkafka_create_consumer(group_id2);
    libnjkafka_consumer_subscribe(consumer2, strdup(kafka_topic));

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

    libnjkafka_consumer_close(consumer);
    libnjkafka_consumer_close(consumer2);

    libnjkafka_teardown();
    return 0;
}
