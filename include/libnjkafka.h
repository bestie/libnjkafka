#ifndef LIBNJKAFKA_STRUCTS_H
#define LIBNJKAFKA_H

#include <libnjkafka_structs.h>
#include <libnjkafka_callbacks.h>

int libnjkafka_init();
int libnjkafka_init_thread();
int libnjkafka_teardown_thread();
int libnjkafka_teardown();


libnjkafka_Consumer* libnjkafka_create_consumer(char* group_id);
libnjkafka_Consumer* libnjkafka_create_consumer_with_config(libnjkafka_ConsumerConfig* config);
libnjkafka_ConsumerRecord_List* libnjkafka_consumer_poll(libnjkafka_Consumer* consumer, int timeout_ms);
libnjkafka_BatchResults libnjkafka_consumer_poll_each_message(libnjkafka_Consumer* consumer, int timeout_ms, libnjkafka_ConsumerRecordProcessor* message_processor, void* opaque);
int libnjkafka_consumer_subscribe(libnjkafka_Consumer* consumer, char* topic);
int libnjkafka_consumer_commit_all_sync(libnjkafka_Consumer* consumer, int timeout_ms);
int libnjkafka_consumer_close(libnjkafka_Consumer* consumer);

#endif
