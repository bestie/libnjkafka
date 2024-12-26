#include <ruby.h>
#include "libnjkafka.h"  // Assuming you have the libnjkafka header file available
#include "libnjkafka_callbacks.h"

typedef struct {
    libnjkafka_Consumer* consumer_ref;
} Consumer;

VALUE module;
VALUE consumer_class;

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

static VALUE consumer_subscribe(VALUE self, VALUE kafka_topic) {
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

    int result = libnjkafka_consumer_subscribe(consumer->consumer_ref, c_kafka_topic);
    if (result != 0) {
        rb_raise(rb_eRuntimeError, "Failed to subscribe to Kafka topic");
    }

    return Qnil;
}

static VALUE consumer_poll(VALUE self, VALUE timeout) {
    Consumer* consumer;
    Data_Get_Struct(self, Consumer, consumer);
    Check_Type(timeout, T_FIXNUM);
    int c_timeout = NUM2INT(timeout);

    libnjkafka_ConsumerRecord_List* record_list = libnjkafka_consumer_poll(consumer->consumer_ref, c_timeout);

    // Just Ruby hashes for now
    VALUE array = rb_ary_new2(record_list->count);
    for (int i = 0; i < record_list->count; i++) {
        VALUE hash = rb_hash_new();
        rb_hash_aset(hash, rb_str_new_cstr("partition"), INT2NUM(record_list->records[i].partition));
        rb_hash_aset(hash, rb_str_new_cstr("offset"), LONG2NUM(record_list->records[i].offset));
        rb_hash_aset(hash, rb_str_new_cstr("timestamp"), LONG2NUM(record_list->records[i].timestamp));
        rb_hash_aset(hash, rb_str_new_cstr("key"), rb_str_new_cstr(record_list->records[i].key));
        rb_hash_aset(hash, rb_str_new_cstr("topic"), rb_str_new_cstr(record_list->records[i].topic));
        rb_hash_aset(hash, rb_str_new_cstr("value"), rb_str_new_cstr(record_list->records[i].value));
        rb_ary_push(array, hash);
    }

    return array;
}

static int poll_each_message_callback(libnjkafka_ConsumerRecord record, VALUE block) {
    VALUE hash = rb_hash_new();
    rb_hash_aset(hash, rb_str_new_cstr("partition"), INT2NUM(record.partition));
    rb_hash_aset(hash, rb_str_new_cstr("offset"), LONG2NUM(record.offset));
    rb_hash_aset(hash, rb_str_new_cstr("timestamp"), LONG2NUM(record.timestamp));
    rb_hash_aset(hash, rb_str_new_cstr("key"), rb_str_new_cstr(record.key));
    rb_hash_aset(hash, rb_str_new_cstr("topic"), rb_str_new_cstr(record.topic));
    rb_hash_aset(hash, rb_str_new_cstr("value"), rb_str_new_cstr(record.value));

    rb_proc_call(block, rb_ary_new_from_args(1, hash));

    return 0;
}

static void consumer_free(void* ptr) {
    Consumer* consumer = (Consumer*)ptr;
    free(consumer);
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

    libnjkafka_Consumer* consumer_ref = libnjkafka_create_consumer_with_config(&config);
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

    rb_define_singleton_method(module, "create_consumer", create_consumer, 1);

    rb_define_alloc_func(consumer_class, consumer_alloc);
    rb_define_method(consumer_class, "initialize", consumer_initialize, 0);
    rb_define_method(consumer_class, "subscribe", consumer_subscribe, 1);
    rb_define_method(consumer_class, "poll", consumer_poll, 1);
    rb_define_method(consumer_class, "poll_each_message", consumer_poll_each_message, 1);
    rb_define_method(consumer_class, "commit_all_sync", consumer_commit_all_sync, 1);
    rb_define_method(consumer_class, "close", consumer_close, 0);
}
