#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "libnjkafka_structs.h"
#include "libnjkafka_callbacks.h"
#include "libnjkafka_core.h"

#define INFO "\x1B[1;36m"
#define RESET "\x1B[0;0m"

graal_isolate_t* graalvm_isolate;
__thread graal_isolatethread_t* graalvm_thread_isolate = NULL;

int libnjkafka_init() {
    int result = graal_create_isolate(NULL, &graalvm_isolate, &graalvm_thread_isolate);
    if (result != 0)
    {
        fprintf(stderr, "Failed to create GraalVM isolate (code %d)\n", result);
    }
    return result;
};

int libnjkafka_init_thread() {
    if (graalvm_thread_isolate != NULL) {
        fprintf(stderr, "Thread isolate already initialized\n");
        return 1;
    }

    fprintf(stderr, "Attaching thread\n");
    int result = graal_attach_thread(graalvm_isolate, &graalvm_thread_isolate);
    if(result != 0) {
        fprintf(stderr, "Failed to attach thread to isolate (code %d)\n", result);
    }
    return result;
}

int libnjkafka_teardown_thread() {
    if (graalvm_thread_isolate == NULL) {
        fprintf(stderr, "Thread isolate already detached\n");
        return 1;
    }

    int result = graal_detach_thread(graalvm_thread_isolate);
    if(result != 0) {
        fprintf(stderr, "Failed to detach thread from isolate (code %d)\n", result);
        return result;
    }
    return result;
}

int libnjkafka_teardown() {
    int result = graal_tear_down_isolate(graalvm_thread_isolate);
    if (result != 0) {
        fprintf(stderr, "Failed tear down GraalVM isolate (code %d)\n", result);
    }
    return result;
}

libnjkafka_Producer* libnjkafka_create_producer(libnjkafka_ProducerConfig* config) {
    printf("Creating consumer with config\n");

    long producer_id = libnjkafka_java_create_producer(graalvm_thread_isolate, config);

    libnjkafka_Producer* producer = (libnjkafka_Producer*) malloc(sizeof(libnjkafka_Producer));
    producer->id = producer_id;
    return producer;
}

int libnjkafka_producer_send(libnjkafka_Producer* producer, libnjkafka_ProducerRecord* record) {
    printf("C-API: Sending record to producer %ld, topic=%s,message=%s,partition=%d\n", producer->id, record->topic, record->value, record->partition);

    long result = libnjkafka_java_producer_send(graalvm_thread_isolate, producer->id, record);

    return result;
}

int libnjkafka_producer_close(libnjkafka_Producer* producer) {
    long result = libnjkafka_java_producer_close(graalvm_thread_isolate, producer->id);

    return result;
}

libnjkafka_Consumer* libnjkafka_create_consumer(libnjkafka_ConsumerConfig* config) {
    printf("Creating consumer with config\n");

    long consumer_id = libnjkafka_java_create_consumer(graalvm_thread_isolate, config);

    libnjkafka_Consumer* consumer = (libnjkafka_Consumer*) malloc(sizeof(libnjkafka_Consumer));
    consumer->id = consumer_id;
    return consumer;
}

int libnjkafka_consumer_subscribe(libnjkafka_Consumer* consumer, char* topic, libnjkafka_ConsumerRebalanceListener* rebalance_listener) {
    printf("Subscribing consumer %ld to topic: %s\n", consumer->id, topic);

    long result = libnjkafka_java_consumer_subscribe(graalvm_thread_isolate, consumer->id, topic, rebalance_listener);

    return result;
}

int libnjkafka_consumer_commit_all_sync(libnjkafka_Consumer* consumer, int timeout_ms) {
    printf("Committing all offsets\n");

    int result = libnjkafka_java_consumer_commit_all_sync(graalvm_thread_isolate, consumer->id, timeout_ms);

    return result;
}

libnjkafka_ConsumerRecord_List* libnjkafka_consumer_poll(libnjkafka_Consumer* consumer, int timeout_ms) {
    libnjkafka_ConsumerRecord_List* records = libnjkafka_java_consumer_poll(graalvm_thread_isolate, consumer->id, timeout_ms);

    return records;
}

// Preferred callback pattern, handles memory management and commiting of offsets
libnjkafka_BatchResults libnjkafka_consumer_poll_each_message(libnjkafka_Consumer* consumer, int timeout_ms, libnjkafka_ConsumerRecordProcessor message_processor, void* opaque) {
    printf("poll_each_message: Polling for records\n");

    libnjkafka_ConsumerRecord_List* records = libnjkafka_java_consumer_poll(graalvm_thread_isolate, consumer->id, timeout_ms);
    libnjkafka_BatchResults results = { .total_records = records->count, .success_count = 0 };

    for(int i = 0; i < records->count; i++) {
        libnjkafka_ConsumerRecord record = records->records[i];

        int result = message_processor(record, opaque);

        if (result != 0) {
            printf("Failed to process message, bailing! error=%d\n", result);
            break;
        }
        results.success_count++;
    }

    if (results.success_count == results.total_records) {
        printf("Processed all messages successfully\n");
        // use same timeout to commit
        libnjkafka_consumer_commit_all_sync(consumer, timeout_ms);
    } else {
        printf("Processed %d of %d messages successfully\n", results.success_count, results.total_records);
    }

    free(records);
    return results;
}

libnjkafka_TopicPartition_List* libnjkafka_consumer_assignment(libnjkafka_Consumer* consumer) {
    printf("Getting consumer assignment\n");

    libnjkafka_TopicPartition_List* topic_partitions = libnjkafka_java_consumer_assignment(graalvm_thread_isolate, consumer->id);

    return topic_partitions;
}

libnjkafka_TopicPartitionOffsetAndMetadata_List* libnjkafka_consumer_committed(libnjkafka_Consumer* consumer, libnjkafka_TopicPartition_List* topic_partitions, int timeout_ms) {
    printf("Getting committed offsets\n");

    libnjkafka_TopicPartitionOffsetAndMetadata_List* offsets = libnjkafka_java_consumer_committed(graalvm_thread_isolate, consumer->id, topic_partitions, timeout_ms);

    return offsets;
}

int libnjkafka_consumer_close(libnjkafka_Consumer* consumer) {
    printf("Closing consumer\n");

    int result = libnjkafka_java_consumer_close(graalvm_thread_isolate, consumer->id);
    free(consumer);

    return result;
}

void libnjkafka_free_ConsumerRecord(libnjkafka_ConsumerRecord* record) {
  /*free(record->key);*/
  /*free(record->topic);*/
  /*free(record->value);*/
  /*free(record);*/
}

void libnjkafka_free_ConsumerRecord_List(libnjkafka_ConsumerRecord_List* list) {
  for(int i = 0; i < list->count; i++) {
    libnjkafka_free_ConsumerRecord(&list->records[i]);
  }
  free(list);
}

void libnjkafka_free_ProducerRecord(libnjkafka_ProducerRecord* record) {
  free(record->key);
  free(record->topic);
  free(record->value);
  free(record);
}


void libnjkafka_free_TopicPartition(libnjkafka_TopicPartition* tp) {
  /*free(tp->topic);*/
}

void libnjkafka_free_TopicPartition_List(libnjkafka_TopicPartition_List* list) {
  for(int i = 0; i < list->count; i++) {
    libnjkafka_free_TopicPartition(&list->items[i]);
  }
  free(list);
}
