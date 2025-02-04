#ifndef LIBNJKAFKA_CALLBACKS_H
#define LIBNJKAFKA_CALLBACKS_H

#include "libnjkafka_structs.h"

typedef int (*libnjkafka_ConsumerRecordProcessor)(libnjkafka_ConsumerRecord, void*);
typedef void (*libnjkafka_ConsumerRebalanceCallback)(void* gvm_thread, libnjkafka_TopicPartition_List*);

typedef struct {
  libnjkafka_ConsumerRebalanceCallback on_partitions_assigned;
  libnjkafka_ConsumerRebalanceCallback on_partitions_revoked;
  libnjkafka_ConsumerRebalanceCallback on_partitions_lost;
} libnjkafka_ConsumerRebalanceListener;

#endif
