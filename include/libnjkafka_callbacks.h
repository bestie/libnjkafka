#ifndef LIBNJKAFKA_CALLBACKS_H
#define LIBNJKAFKA_CALLBACKS_H

#include "libnjkafka_structs.h"

typedef int (*libnjkafka_ConsumerRecordProcessor)(libnjkafka_ConsumerRecord, void*);

#endif
