#include <ruby.h>
#include "libnjkafka.h"  // Assuming you have the libnjkafka header file available

typedef struct {
    libnjkafka_Consumer* consumer_ref;
} Consumer;

VALUE module;
VALUE consumer_class;

void hash_to_consumer_config(VALUE hash, libnjkafka_ConsumerConfig* config) {
    Check_Type(hash, T_HASH);

    VALUE val;

    val = rb_hash_aref(hash, ID2SYM(rb_intern("bootstrap_servers")));
    config->bootstrap_servers = NIL_P(val) ? NULL : StringValueCStr(val);

    val = rb_hash_aref(hash, ID2SYM(rb_intern("group_id")));
    config->group_id = NIL_P(val) ? NULL : StringValueCStr(val);

    val = rb_hash_aref(hash, ID2SYM(rb_intern("enable_auto_commit")));
    config->enable_auto_commit = NIL_P(val) ? 0 : RTEST(val);

    val = rb_hash_aref(hash, ID2SYM(rb_intern("auto_commit_interval_ms")));
    config->auto_commit_interval_ms = NIL_P(val) ? 0 : NUM2INT(val);

    val = rb_hash_aref(hash, ID2SYM(rb_intern("offset_reset")));
    config->offset_reset = NIL_P(val) ? NULL : StringValueCStr(val);

    val = rb_hash_aref(hash, ID2SYM(rb_intern("session_timeout_ms")));
    config->session_timeout_ms = NIL_P(val) ? 0 : NUM2INT(val);

    val = rb_hash_aref(hash, ID2SYM(rb_intern("request_timeout_ms")));
    config->request_timeout_ms = NIL_P(val) ? 0 : NUM2INT(val);

    val = rb_hash_aref(hash, ID2SYM(rb_intern("fetch_min_bytes")));
    config->fetch_min_bytes = NIL_P(val) ? 0 : NUM2INT(val);

    val = rb_hash_aref(hash, ID2SYM(rb_intern("fetch_max_wait_ms")));
    config->fetch_max_wait_ms = NIL_P(val) ? 0 : NUM2INT(val);

    val = rb_hash_aref(hash, ID2SYM(rb_intern("max_partition_fetch_bytes")));
    config->max_partition_fetch_bytes = NIL_P(val) ? 0 : NUM2INT(val);

    val = rb_hash_aref(hash, ID2SYM(rb_intern("isolation_level")));
    config->isolation_level = NIL_P(val) ? NULL : StringValueCStr(val);

    val = rb_hash_aref(hash, ID2SYM(rb_intern("max_poll_records")));
    config->max_poll_records = NIL_P(val) ? 0 : NUM2INT(val);

    val = rb_hash_aref(hash, ID2SYM(rb_intern("max_poll_interval_ms")));
    config->max_poll_interval_ms = NIL_P(val) ? 0 : NUM2INT(val);

    val = rb_hash_aref(hash, ID2SYM(rb_intern("auto_offset_reset")));
    config->auto_offset_reset = NIL_P(val) ? NULL : StringValueCStr(val);

    val = rb_hash_aref(hash, ID2SYM(rb_intern("client_id")));
    config->client_id = NIL_P(val) ? NULL : StringValueCStr(val);

    val = rb_hash_aref(hash, ID2SYM(rb_intern("heartbeat_interval_ms")));
    config->heartbeat_interval_ms = NIL_P(val) ? 0 : NUM2INT(val);

    val = rb_hash_aref(hash, ID2SYM(rb_intern("check_crcs")));
    config->check_crcs = NIL_P(val) ? 0 : RTEST(val);

    val = rb_hash_aref(hash, ID2SYM(rb_intern("fetch_max_bytes")));
    config->fetch_max_bytes = NIL_P(val) ? 0 : NUM2INT(val);
}

static void consumer_free(void* ptr) {
    Consumer* consumer = (Consumer*)ptr;
    free(consumer);
}

static VALUE consumer_alloc(VALUE klass) {
    Consumer* consumer = ALLOC(Consumer);
    return Data_Wrap_Struct(klass, NULL, consumer_free, consumer);
}

// static VALUE consumer_initialize(VALUE self, VALUE group_id) {
//     return self;
// }

static VALUE consumer_close(VALUE self) {
    Consumer* consumer;
    Data_Get_Struct(self, Consumer, consumer);

    int result = libnjkafka_consumer_close(consumer->consumer_ref);
    if (result != 0) {
        rb_raise(rb_eRuntimeError, "Failed to close Kafka consumer");
    }

    return Qnil;
}

static VALUE consumer_subscribe(VALUE self, VALUE kafka_topic) {
    Consumer* consumer;
    Data_Get_Struct(self, Consumer, consumer);

    Check_Type(kafka_topic, T_STRING);
    char* c_kafka_topic = StringValueCStr(kafka_topic);

    if (consumer->consumer_ref->id == NULL) {
        rb_raise(rb_eRuntimeError, "Consumer reference is NULL");
    } else {
        printf("Consumer ref: %ld\n", consumer->consumer_ref->id);
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

static void consumer_poll_each_message(VALUE self, VALUE timeout) {
    rb_raise(rb_eArgError, "Not implemented 🤷");
}

static VALUE create_consumer(VALUE self, VALUE config_hash) {
    Check_Type(config_hash, T_HASH);

    VALUE group_id = rb_hash_aref(config_hash, ID2SYM(rb_intern("group_id")));
    printf("Group ID: %s\n", StringValueCStr(group_id));

    // free this at some point maybe?
    libnjkafka_ConsumerConfig* config = (libnjkafka_ConsumerConfig*)malloc(sizeof(libnjkafka_ConsumerConfig));
    // hash_to_consumer_config(config_hash, config);

    config->group_id = NIL_P(group_id) ? NULL : StringValueCStr(group_id);
    config->bootstrap_servers = strdup("localhost:9092");
    config->enable_auto_commit = 1;
    config->auto_commit_interval_ms = 5000;
    config->offset_reset = strdup("earliest");
    config->session_timeout_ms = 10000;
    config->request_timeout_ms = 30000;
    config->fetch_min_bytes = 1;
    config->fetch_max_wait_ms = 500;
    config->max_partition_fetch_bytes = 1048576;
    config->isolation_level = strdup("read_committed");
    config->max_poll_records = 500;
    config->max_poll_interval_ms = 300000;
    config->auto_offset_reset = strdup("earliest");
    config->client_id = strdup("my-client");
    config->heartbeat_interval_ms = 3000;
    config->check_crcs = 1;
    config->fetch_max_bytes = 52428800;

    libnjkafka_Consumer* consumer_ref = libnjkafka_create_consumer_with_config(config);
    if (consumer_ref == NULL || consumer_ref->id == -1) {
        rb_raise(rb_eRuntimeError, "Failed to create Kafka consumer");
    }
    printf("Consumer ref id: %ld\n", consumer_ref->id);

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
    // rb_define_method(consumer_class, "initialize", consumer_initialize, 0);
    rb_define_method(consumer_class, "subscribe", consumer_subscribe, 1);
    rb_define_method(consumer_class, "poll", consumer_poll, 1);
    rb_define_method(consumer_class, "poll_each_message", consumer_poll_each_message, 1);
    rb_define_method(consumer_class, "close", consumer_close, 0);
}
