#include <ruby.h>
#include <ruby/util.h>
#include <ruby/thread.h>
#include <pthread.h>
#include <unistd.h>
#include "libnjkafka.h"
#include "libnjkafka_callbacks.h"

typedef struct {
    VALUE listener_obj;
    VALUE consumer_obj;
} RebalanceListenerOpaque;

typedef struct {
    VALUE listener_obj;
    const char* method_name;
    libnjkafka_TopicPartition_List* topic_partition_list;
} RebalanceEvent;


typedef struct {
    libnjkafka_Consumer* consumer_ref;
    bool closed;
    void* pending_rebalance_event;
    bool queue_rebalance_events;
    RebalanceEvent** rebalance_queue;
    int rebalance_queue_size;
} Consumer;

static void consumer_free(VALUE consumer) {
    // not necessary
}

static const rb_data_type_t consumer_data_type = {
    .wrap_struct_name = "Consumer",
    .function = {
        .dmark = NULL,
        .dfree = (void (*)(void*))consumer_free,
    },
    .data = NULL,
    .flags = RUBY_TYPED_FREE_IMMEDIATELY
};


VALUE module;
VALUE consumer_class;
VALUE topic_partition_class;
VALUE topic_partition_list_class;
VALUE consumer_record_class;
VALUE consumer_record_list_class;

// Define rebalancer method names
#define ON_PARTITIONS_ASSIGNED "on_partitions_assigned"
#define ON_PARTITIONS_REVOKED "on_partitions_revoked"
#define ON_PARTITIONS_LOST "on_partitions_lost"
#define MAX_REBALANCE_EVENTS 32

// Helper methods
static char* get_hash_string_value(VALUE hash, const char* key) {
    VALUE val = rb_hash_aref(hash, ID2SYM(rb_intern(key)));
    return NIL_P(val) ? NULL : ruby_strdup(StringValueCStr(val));
}

static int get_hash_int_value(VALUE hash, const char* key) {
    VALUE val = rb_hash_aref(hash, ID2SYM(rb_intern(key)));
    return NIL_P(val) ? 0 : NUM2INT(val);
}

static int get_hash_bool_value(VALUE hash, const char* key) {
    VALUE val = rb_hash_aref(hash, ID2SYM(rb_intern(key)));
    return NIL_P(val) ? 0 : RTEST(val);
}

static void call_rebalance_listener(RebalanceEvent* event, VALUE consumer_obj) {
    VALUE target_method = rb_intern(event->method_name);
    VALUE listener_obj = event->listener_obj;
    libnjkafka_TopicPartition_List* topic_partitions = event->topic_partition_list;

    if(!rb_respond_to(listener_obj, target_method)) return;

    VALUE rb_array = rb_ary_new2(topic_partitions->count);
    for (int i = 0; i < topic_partitions->count; i++) {
        VALUE topic_name = rb_str_new_cstr(topic_partitions->items[i].topic);
        printf(" Topic: %s, Partition: %d\n", topic_partitions->items[i].topic, topic_partitions->items[i].partition);
        VALUE partition_n = INT2NUM(topic_partitions->items[i].partition);

        // create a new TopicPartition object
        VALUE rb_topic_partion = rb_funcall(topic_partition_class, rb_intern("new"), 2, topic_name, partition_n);
        rb_ary_push(rb_array, rb_topic_partion);
    }
    VALUE rb_topic_parition_list = rb_funcall(topic_partition_list_class, rb_intern("new"), 1, rb_array);

    rb_funcall(listener_obj, target_method, 2, consumer_obj, rb_topic_parition_list);

    xfree(event);
    libnjkafka_free_TopicPartition_List(topic_partitions);
}

static void add_pending_rebalance_event(const char* method_name, libnjkafka_TopicPartition_List* topic_partition_list, void* opaque) {
    RebalanceListenerOpaque* rlbo = (RebalanceListenerOpaque*)opaque;

    RebalanceEvent* event = ALLOC(RebalanceEvent);
    event->listener_obj = rlbo->listener_obj;
    event->topic_partition_list = topic_partition_list;
    event->method_name = method_name;

    Consumer* consumer;
    TypedData_Get_Struct(rlbo->consumer_obj, Consumer, &consumer_data_type, consumer);

    if(consumer->queue_rebalance_events) {
        if(consumer->rebalance_queue_size < MAX_REBALANCE_EVENTS) {
            consumer->rebalance_queue[consumer->rebalance_queue_size] = event;
            consumer->rebalance_queue_size++;
        } else {
            fprintf(stderr, "Dropped rebalance event. Too many rebalance events in queue.");
        }
    } else {
        call_rebalance_listener(event, rlbo->consumer_obj);
    }
}

static void on_partitions_assigned(void* gvm_thread, libnjkafka_TopicPartition_List* topic_partitions, void* opaque) {
    printf("C-EXT Assigned partitions: %d\n", topic_partitions->count);
    for(int i=0; i<topic_partitions->count; i++) {
      printf(" C-EXT 👂👂👂 assigned Topic `%s`, Partition: %d\n", topic_partitions->items[i].topic, topic_partitions->items[i].partition);
    }

    add_pending_rebalance_event(ON_PARTITIONS_ASSIGNED, topic_partitions, opaque);
}

static void on_partitions_revoked(void* gvm_thread, libnjkafka_TopicPartition_List* topic_partitions, void* opaque) {
    printf("C-EXT Revoked partitions: %d\n", topic_partitions->count);
    for(int i=0; i<topic_partitions->count; i++) {
      printf(" C-EXT 👂👂👂 revoked Topic `%s`, Partition: %d\n", topic_partitions->items[i].topic, topic_partitions->items[i].partition);
    }

    add_pending_rebalance_event(ON_PARTITIONS_REVOKED, topic_partitions, opaque);

}

static void on_partitions_lost(void* gvm_thread, libnjkafka_TopicPartition_List* topic_partitions, void* opaque) {
    printf("C-EXT Lost partitions: %d\n", topic_partitions->count);
    for(int i=0; i<topic_partitions->count; i++) {
      printf(" C-EXT 👂👂👂 lost Topic `%s`, Partition: %d\n", topic_partitions->items[i].topic, topic_partitions->items[i].partition);
    }

    add_pending_rebalance_event(ON_PARTITIONS_LOST, topic_partitions, opaque);
}

libnjkafka_ConsumerConfig hash_to_consumer_config(VALUE hash) {
    Check_Type(hash, T_HASH);

    libnjkafka_ConsumerConfig config;

    config.auto_commit_interval_ms = get_hash_int_value(hash, "auto_commit_interval_ms");
    config.auto_offset_reset = get_hash_string_value(hash, "auto_offset_reset");
    config.bootstrap_servers = get_hash_string_value(hash, "bootstrap_servers");
    config.check_crcs = get_hash_bool_value(hash, "check_crcs");
    config.client_id = get_hash_string_value(hash, "client_id");
    config.enable_auto_commit = get_hash_bool_value(hash, "enable_auto_commit");
    config.fetch_max_bytes = get_hash_int_value(hash, "fetch_max_bytes");
    config.fetch_max_wait_ms = get_hash_int_value(hash, "fetch_max_wait_ms");
    config.fetch_min_bytes = get_hash_int_value(hash, "fetch_min_bytes");
    config.group_id = get_hash_string_value(hash, "group_id");
    config.heartbeat_interval_ms = get_hash_int_value(hash, "heartbeat_interval_ms");
    config.isolation_level = get_hash_string_value(hash, "isolation_level");
    config.max_partition_fetch_bytes = get_hash_int_value(hash, "max_partition_fetch_bytes");
    config.max_poll_interval_ms = get_hash_int_value(hash, "max_poll_interval_ms");
    config.max_poll_records = get_hash_int_value(hash, "max_poll_records");
    config.offset_reset = get_hash_string_value(hash, "offset_reset");
    config.request_timeout_ms = get_hash_int_value(hash, "request_timeout_ms");
    config.session_timeout_ms = get_hash_int_value(hash, "session_timeout_ms");

    return config;
}

static VALUE consumer_subscribe(VALUE self, VALUE kafka_topic, VALUE rb_rebalance_listener) {
    Consumer* consumer;
    TypedData_Get_Struct(self, Consumer, &consumer_data_type, consumer);

    Check_Type(kafka_topic, T_STRING);
    char* c_kafka_topic = StringValueCStr(kafka_topic);

    if (consumer->consumer_ref == NULL) {
        rb_raise(rb_eRuntimeError, "Consumer reference is NULL");
    } else {
        fprintf(stderr, "Consumer ref: %ld\n", consumer->consumer_ref->id);
    }

    if (c_kafka_topic == NULL || c_kafka_topic[strlen(c_kafka_topic)] != '\0') {
        rb_raise(rb_eRuntimeError, "Invalid topic string");
    }

    libnjkafka_ConsumerRebalanceListener* rebalance_listener;

    if (rb_rebalance_listener == Qnil) {
        rebalance_listener = libnjkafka_null_rebalance_listener();
    } else {
        rebalance_listener = ALLOC(libnjkafka_ConsumerRebalanceListener);
        RebalanceListenerOpaque* opaque;
        opaque = ALLOC(RebalanceListenerOpaque);
        opaque->listener_obj = rb_rebalance_listener;
        opaque->consumer_obj = self;

        rebalance_listener->on_partitions_assigned = (libnjkafka_ConsumerRebalanceCallback)&on_partitions_assigned;
        rebalance_listener->on_partitions_revoked = (libnjkafka_ConsumerRebalanceCallback)&on_partitions_revoked;
        rebalance_listener->on_partitions_lost = (libnjkafka_ConsumerRebalanceCallback)&on_partitions_lost;
        rebalance_listener->opaque = (void*)opaque;
    }

    int result = libnjkafka_consumer_subscribe(consumer->consumer_ref, c_kafka_topic, rebalance_listener);
    if (result != 0) {
        rb_raise(rb_eRuntimeError, "Failed to subscribe to Kafka topic");
    }

    return Qnil;
}

static VALUE consumer_record_to_rb_hash(libnjkafka_ConsumerRecord record) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, ID2SYM(rb_intern("partition")), INT2NUM(record.partition));
    rb_hash_aset(hash, ID2SYM(rb_intern("offset")), LONG2NUM(record.offset));
    rb_hash_aset(hash, ID2SYM(rb_intern("timestamp")), LONG2NUM(record.timestamp));
    rb_hash_aset(hash, ID2SYM(rb_intern("key")), rb_str_new_cstr(record.key));
    rb_hash_aset(hash, ID2SYM(rb_intern("topic")), rb_str_new_cstr(record.topic));
    rb_hash_aset(hash, ID2SYM(rb_intern("value")), rb_str_new_cstr(record.value));
    return hash;
}

static VALUE consumer_record_to_rb(libnjkafka_ConsumerRecord record) {
    VALUE kwargs = consumer_record_to_rb_hash(record);
    VALUE argv[1] = { kwargs };
    VALUE cr = rb_funcallv_kw(consumer_record_class, rb_intern("new"), 1, argv, RB_PASS_KEYWORDS);
    return cr;
}

typedef struct {
    libnjkafka_Consumer* consumer;
    int timeout;
    libnjkafka_ConsumerRecord_List* results;
} poll_t;

static void* poll_into_struct(void* opaque) {
    poll_t* poll = (poll_t*)opaque;
    poll->results = libnjkafka_consumer_poll(poll->consumer, poll->timeout);

    return opaque;
}

static void consumer_enable_rebalance_queue(Consumer* consumer) {
    RebalanceEvent** queue = ALLOC_N(RebalanceEvent*, MAX_REBALANCE_EVENTS);
    consumer->rebalance_queue = queue;
    consumer->rebalance_queue_size = 0;
    consumer->queue_rebalance_events = true;
}

static void consumer_disable_rebalance_queue(Consumer* consumer) {
    consumer->queue_rebalance_events = false;
    consumer->rebalance_queue_size = 0;
    consumer->rebalance_queue = NULL;
    xfree(consumer->rebalance_queue);
}

static VALUE consumer_poll(VALUE self, VALUE timeout) {
    Consumer* consumer;
    TypedData_Get_Struct(self, Consumer, &consumer_data_type, consumer);
    Check_Type(timeout, T_FIXNUM);
    int c_timeout = NUM2INT(timeout);

    consumer_enable_rebalance_queue(consumer);
    poll_t poll = {
        .consumer = consumer->consumer_ref,
        .timeout = c_timeout,
    };

    libnjkafka_init_thread();
    rb_funcall(module, rb_intern("ensure_current_thread_teardown"), 0);

    rb_thread_call_without_gvl(
        poll_into_struct,
        &poll,
        RUBY_UBF_IO,
        NULL
    );

    libnjkafka_ConsumerRecord_List* record_list = poll.results;

    printf("⚖️⚖️⚖️ There are %d rebalance event(s)\n", consumer->rebalance_queue_size);
    for(int i=0; i < consumer->rebalance_queue_size; i++) {
        call_rebalance_listener(consumer->rebalance_queue[i], self);
    }

    consumer_disable_rebalance_queue(consumer);

    VALUE array = rb_ary_new2(record_list->count);
    for (int i = 0; i < record_list->count; i++) {
        VALUE cr = consumer_record_to_rb(record_list->records[i]);
        rb_ary_push(array, cr);
    }

    VALUE cr_list = rb_funcall(consumer_record_list_class, rb_intern("new"), 1, array);

    libnjkafka_free_ConsumerRecord_List(record_list);
    return cr_list;
}

static int poll_each_message_callback(libnjkafka_ConsumerRecord record, VALUE block) {
    VALUE cr = consumer_record_to_rb(record);
    rb_proc_call(block, rb_ary_new_from_args(1, cr));
    return 0;
}

static VALUE consumer_alloc(VALUE klass) {
    Consumer* consumer = ALLOC(Consumer);
    return TypedData_Wrap_Struct(consumer_class, &consumer_data_type, consumer);

}

static VALUE consumer_initialize(VALUE self) {
     return self;
}

static VALUE consumer_commit_all_sync(VALUE self, VALUE timeout_ms) {
    Consumer* consumer;
    TypedData_Get_Struct(self, Consumer, &consumer_data_type, consumer);

    Check_Type(timeout_ms, T_FIXNUM);
    int c_timeout_ms = NUM2INT(timeout_ms);

    int result = libnjkafka_consumer_commit_all_sync(consumer->consumer_ref, c_timeout_ms);
    if (result != 0) {
        rb_raise(rb_eRuntimeError, "Failed to commit all offsets");
    }

    return Qnil;
}

static VALUE consumer_poll_each_message(VALUE self, VALUE timeout_ms) {
    Consumer* consumer;
    TypedData_Get_Struct(self, Consumer, &consumer_data_type, consumer);

    rb_need_block();
    VALUE block = rb_block_proc();

    Check_Type(timeout_ms, T_FIXNUM);
    int c_timeout = NUM2INT(timeout_ms);

    libnjkafka_ConsumerRecordProcessor* block_runner_callback = (libnjkafka_ConsumerRecordProcessor*)poll_each_message_callback;
    void* opaque = (void*)block;

    libnjkafka_BatchResults results = libnjkafka_consumer_poll_each_message(consumer->consumer_ref, c_timeout, block_runner_callback, opaque);
    if (results.success_count == results.total_records) {
        fprintf(stderr, "Processed all messages successfully\n");
    } else {
        fprintf(stderr, "Processed %d of %d messages successfully\n", results.success_count, results.total_records);
    }

    return Qnil;
}

static VALUE create_consumer(VALUE self, VALUE config_hash) {
    Check_Type(config_hash, T_HASH);

    VALUE group_id = rb_hash_aref(config_hash, ID2SYM(rb_intern("group_id")));
    fprintf(stderr, "Group ID: %s\n", StringValueCStr(group_id));

    libnjkafka_ConsumerConfig config = hash_to_consumer_config(config_hash);

    libnjkafka_Consumer* consumer_ref = libnjkafka_create_consumer(&config);
    if (consumer_ref == NULL || consumer_ref->id == -1) {
        rb_raise(rb_eRuntimeError, "Failed to create Kafka consumer");
    }
    fprintf(stderr, "Consumer ref id: %ld\n", consumer_ref->id);

    Consumer* consumer = ALLOC(Consumer);
    consumer->consumer_ref = consumer_ref;
    consumer->rebalance_queue_size = 0;
    consumer->queue_rebalance_events = false;

    return TypedData_Wrap_Struct(consumer_class, &consumer_data_type, consumer);
}

static VALUE consumer_close(VALUE self) {
    Consumer* consumer;
    TypedData_Get_Struct(self, Consumer, &consumer_data_type, consumer);

    int result = libnjkafka_consumer_close(consumer->consumer_ref);
    if (result != 0) {
        rb_raise(rb_eRuntimeError, "Failed to close Kafka consumer");
    }

    return Qnil;
}

static void end_proc_teardown(VALUE data) {
    libnjkafka_teardown();
}

static VALUE mod_teardown(VALUE value) {
    libnjkafka_teardown();
    return value;
}

static VALUE mod_teardown_current_thread(VALUE value) {
    libnjkafka_teardown_thread();
    return value;
}

void Init_libnjkafka_ext() {
    libnjkafka_init();
    rb_set_end_proc(end_proc_teardown, Qnil);

    module = rb_define_module("LibNJKafka");
    consumer_class = rb_define_class_under(module, "Consumer", rb_cObject);
    topic_partition_class = rb_define_class_under(module, "TopicPartition", rb_cObject);
    topic_partition_list_class = rb_define_class_under(module, "TopicPartitionList", rb_cObject);
    consumer_record_class =  rb_define_class_under(module, "ConsumerRecord", rb_cObject);
    consumer_record_list_class =  rb_define_class_under(module, "ConsumerRecordList", rb_cObject);

    rb_define_singleton_method(module, "create_consumer", create_consumer, 1);
    rb_define_singleton_method(module, "teardown", mod_teardown, 0);
    rb_define_singleton_method(module, "teardown_current_thread", mod_teardown_current_thread, 0);

    rb_define_alloc_func(consumer_class, consumer_alloc);
    rb_define_method(consumer_class, "initialize", consumer_initialize, 0);
    rb_define_method(consumer_class, "poll", consumer_poll, 1);
    rb_define_method(consumer_class, "poll_each_message", consumer_poll_each_message, 1);
    rb_define_method(consumer_class, "commit_all_sync", consumer_commit_all_sync, 1);
    rb_define_method(consumer_class, "close", consumer_close, 0);
    rb_define_private_method(consumer_class, "cext_subscribe", consumer_subscribe, 2);
}
