#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "libnjkafka_structs.h"
#include "libnjkafka_callbacks.h"
#include "libnjkafka_core.h"

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

libnjkafka_Consumer* libnjkafka_create_consumer(char* group_id) {
    printf("Creating consumer with group_id: %s\n", group_id);

    long consumer_id = libnjkafka_java_create_consumer(graalvm_thread_isolate, group_id);

    libnjkafka_Consumer* consumer = (libnjkafka_Consumer*) malloc(sizeof(libnjkafka_Consumer));
    consumer->id = consumer_id;
    return consumer;
}

libnjkafka_Consumer* libnjkafka_create_consumer_with_config(libnjkafka_ConsumerConfig* config) {
    printf("Creating consumer with config\n");

    long consumer_id = libnjkafka_java_create_consumer_with_config(graalvm_thread_isolate, config);

    libnjkafka_Consumer* consumer = (libnjkafka_Consumer*) malloc(sizeof(libnjkafka_Consumer));
    consumer->id = consumer_id;
    return consumer;
}

int libnjkafka_consumer_subscribe(libnjkafka_Consumer* consumer, char* topic) {
    printf("Subscribing consumer %ld to topic: %s\n", consumer->id, topic);

    long result = libnjkafka_java_consumer_subscribe(graalvm_thread_isolate, consumer->id, topic);

    return result;
}

int libnjkafka_consumer_commit_all_sync(libnjkafka_Consumer* consumer, int timeout_ms) {
    printf("Committing all offsets\n");

    int result = libnjkafka_java_consumer_commit_all_sync(graalvm_thread_isolate, consumer->id, timeout_ms);

    return result;
}

libnjkafka_ConsumerRecord_List* libnjkafka_consumer_poll(libnjkafka_Consumer* consumer, int timeout_ms) {
    printf("Polling for records\n");

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

int libnjkafka_consumer_close(libnjkafka_Consumer* consumer) {
    printf("Closing consumer\n");

    int result = libnjkafka_java_consumer_close(graalvm_thread_isolate, consumer->id);
    free(consumer);

    return result;
}
