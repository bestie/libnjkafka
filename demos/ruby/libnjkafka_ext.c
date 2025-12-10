#include <ruby.h>
#include "libnjkafka.h"  // Assuming you have the libnjkafka header file available
#include "libnjkafka_callbacks.h"

typedef struct {
    libnjkafka_Consumer* consumer_ref;
} Consumer;

typedef struct {
    VALUE listener_obj;
    VALUE consumer_obj;
} RebalanceListenerOpaque;

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

// Helper methods
static char* get_hash_string_value(VALUE hash, const char* key) {
    VALUE val = rb_hash_aref(hash, ID2SYM(rb_intern(key)));
    return NIL_P(val) ? NULL : strdup(StringValueCStr(val));
}

static int get_hash_int_value(VALUE hash, const char* key) {
    VALUE val = rb_hash_aref(hash, ID2SYM(rb_intern(key)));
    return NIL_P(val) ? 0 : NUM2INT(val);
}

static int get_hash_bool_value(VALUE hash, const char* key) {
    VALUE val = rb_hash_aref(hash, ID2SYM(rb_intern(key)));
    return NIL_P(val) ? 0 : RTEST(val);
}

static void call_listener_method(void* rb_objs, const char* method_name, libnjkafka_TopicPartition_List* topic_partitions) {
    VALUE target_method = rb_intern(method_name);
    if(rb_objs == NULL) return;

    RebalanceListenerOpaque* rlbo = (RebalanceListenerOpaque*)rb_objs;
    VALUE listener_obj = rlbo->listener_obj;
    VALUE consumer_obj = rlbo->consumer_obj;

    if(!rb_respond_to(listener_obj, rb_intern(method_name))) return;

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
    libnjkafka_free_TopicPartition_List(topic_partitions);
}

static void on_partitions_assigned(void* gvm_thread, libnjkafka_TopicPartition_List* topic_partitions, void* rb_objs) {
    printf("C-EXT Assigned partitions: %d\n", topic_partitions->count);
    for(int i=0; i<topic_partitions->count; i++) {
      printf(" C-EXT 👂👂👂 assigned Topic `%s`, Partition: %d\n", topic_partitions->items[i].topic, topic_partitions->items[i].partition);
    }

    call_listener_method(rb_objs, ON_PARTITIONS_ASSIGNED, topic_partitions);
}

static void on_partitions_revoked(void* gvm_thread, libnjkafka_TopicPartition_List* topic_partitions, void* rb_objs) {
    printf("C-EXT Revoked partitions: %d\n", topic_partitions->count);
    for(int i=0; i<topic_partitions->count; i++) {
      printf(" C-EXT 👂👂👂 revoked Topic `%s`, Partition: %d\n", topic_partitions->items[i].topic, topic_partitions->items[i].partition);
    }

    call_listener_method(rb_objs, ON_PARTITIONS_REVOKED, topic_partitions);
}

static void on_partitions_lost(void* gvm_thread, libnjkafka_TopicPartition_List* topic_partitions, void* rb_objs) {
    printf("C-EXT Lost partitions: %d\n", topic_partitions->count);
    for(int i=0; i<topic_partitions->count; i++) {
      printf(" C-EXT 👂👂👂 lost Topic `%s`, Partition: %d\n", topic_partitions->items[i].topic, topic_partitions->items[i].partition);
    }

    call_listener_method(rb_objs, ON_PARTITIONS_LOST, topic_partitions);
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
    Data_Get_Struct(self, Consumer, consumer);

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

static VALUE consumer_poll(VALUE self, VALUE timeout) {
    Consumer* consumer;
    Data_Get_Struct(self, Consumer, consumer);
    Check_Type(timeout, T_FIXNUM);
    int c_timeout = NUM2INT(timeout);

    libnjkafka_ConsumerRecord_List* record_list = libnjkafka_consumer_poll(consumer->consumer_ref, c_timeout);

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
    return Data_Wrap_Struct(klass, NULL, consumer_free, consumer);
}

static VALUE consumer_initialize(VALUE self) {
     return self;
}

static VALUE consumer_commit_all_sync(VALUE self, VALUE timeout_ms) {
    Consumer* consumer;
    Data_Get_Struct(self, Consumer, consumer);

    Check_Type(timeout_ms, T_FIXNUM);
    int c_timeout_ms = NUM2INT(timeout_ms);

    int result = libnjkafka_consumer_commit_all_sync(consumer->consumer_ref, c_timeout_ms);
    if (result != 0) {
        rb_raise(rb_eRuntimeError, "Failed to commit all offsets");
    }

    return Qnil;
}

static VALUE consumer_close(VALUE self) {
    Consumer* consumer;
    Data_Get_Struct(self, Consumer, consumer);

    int result = libnjkafka_consumer_close(consumer->consumer_ref);
    if (result != 0) {
        rb_raise(rb_eRuntimeError, "Failed to close Kafka consumer");
    }

    return Qnil;
}

static VALUE consumer_poll_each_message(VALUE self, VALUE timeout_ms) {
    Consumer* consumer;
    Data_Get_Struct(self, Consumer, consumer);

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

    return Data_Wrap_Struct(consumer_class, NULL, consumer_free, consumer);
}

void Init_libnjkafka_ext() {
    libnjkafka_init();

    module = rb_define_module("LibNJKafka");
    consumer_class = rb_define_class_under(module, "Consumer", rb_cObject);
    topic_partition_class = rb_define_class_under(module, "TopicPartition", rb_cObject);
    topic_partition_list_class = rb_define_class_under(module, "TopicPartitionList", rb_cObject);
    consumer_record_class =  rb_define_class_under(module, "ConsumerRecord", rb_cObject);
    consumer_record_list_class =  rb_define_class_under(module, "ConsumerRecordList", rb_cObject);

    rb_define_singleton_method(module, "create_consumer", create_consumer, 1);

    rb_define_alloc_func(consumer_class, consumer_alloc);
    rb_define_method(consumer_class, "initialize", consumer_initialize, 0);
    rb_define_method(consumer_class, "poll", consumer_poll, 1);
    rb_define_method(consumer_class, "poll_each_message", consumer_poll_each_message, 1);
    rb_define_method(consumer_class, "commit_all_sync", consumer_commit_all_sync, 1);
    rb_define_method(consumer_class, "close", consumer_close, 0);
    rb_define_private_method(consumer_class, "cext_subscribe", consumer_subscribe, 2);
}


// your struct
typedef struct {
  void *consumer_ref;
} Consumer;

static void consumer_free(void *p) { xfree(p); }
static size_t consumer_memsize(const void *p) { return sizeof(Consumer); }
// Only if you hold Ruby objects inside Consumer:
// static void consumer_mark(void *p) { rb_gc_mark(((Consumer*)p)->obj); }

static const rb_data_type_t consumer_type = {
  "Consumer",
  { /*mark*/0, consumer_free, consumer_memsize },
  0, 0, RUBY_TYPED_FREE_IMMEDIATELY
};



static VALUE consumer_alloc(VALUE klass) {
  Consumer *c;
  return TypedData_Make_Struct(klass, Consumer, &consumer_type, c);
}

static Consumer *get_consumer(VALUE self) {
  Consumer *c;
  TypedData_Get_Struct(self, Consumer, &consumer_type, c);
  return c;
}

// in Init_...
rb_define_alloc_func(consumer_class, consumer_alloc);
