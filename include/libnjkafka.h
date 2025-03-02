#ifndef LIBNJKAFKA_STRUCTS_H
#define LIBNJKAFKA_H

#include <libnjkafka_structs.h>
#include <libnjkafka_callbacks.h>

int libnjkafka_init();
int libnjkafka_init_thread();
int libnjkafka_teardown_thread();
int libnjkafka_teardown();


libnjkafka_Consumer* libnjkafka_create_consumer(libnjkafka_ConsumerConfig* config);
libnjkafka_ConsumerRecord_List* libnjkafka_consumer_poll(libnjkafka_Consumer* consumer, int timeout_ms);
libnjkafka_BatchResults libnjkafka_consumer_poll_each_message(libnjkafka_Consumer* consumer, int timeout_ms, libnjkafka_ConsumerRecordProcessor* message_processor, void* opaque);
libnjkafka_TopicPartition_List* libnjkafka_consumer_assignment(libnjkafka_Consumer* consumer);
libnjkafka_TopicPartitionOffsetAndMetadata_List* libnjkafka_consumer_committed(libnjkafka_Consumer* consumer, libnjkafka_TopicPartition_List* topic_partition_list, int timeout_ms);
int libnjkafka_consumer_subscribe(libnjkafka_Consumer* consumer, char* topic, libnjkafka_ConsumerRebalanceListener*);
int libnjkafka_consumer_commit_all_sync(libnjkafka_Consumer* consumer, int timeout_ms);
int libnjkafka_consumer_close(libnjkafka_Consumer* consumer);

libnjkafka_Producer* libnjkafka_create_producer(libnjkafka_ProducerConfig* config);
int libnjkafka_producer_send(libnjkafka_Producer* producer, libnjkafka_ProducerRecord* record);
int libnjkafka_producer_close(libnjkafka_Producer* producer);
#endif
