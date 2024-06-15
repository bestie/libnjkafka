# libnjkafka

CURRENTLY AN EXPERIMENTAL PROOF OF CONCEPT


libnjkafka is a "native Java" C compatible client library for Apache Kafka.

Using the magic of GraalVM, the office Java client code is compiled to native code which means:

* C Compatible
* No JVM or GraalVM required
* No significant startup penalty


## C API

The libnjkafka shared object will work with your existing C code.

The public C API wrapper provides convenient functions:

```c
libnjkafka_create_consumer
libnjkafka_consumer_subscribe
libnjkafka_consumer_poll
libnjkafka_consumer_poll_each_message // automatically frees memory and commits offsets
libnjkafka_consumer_commit_all_sync
libnjkafka_consumer_close
```

## Ruby C extension

Included is a Ruby C extension and demo program that executes under the official 

## Go extension

Could be fun.

## Thread Safety

GraalVM provides some safety via its Java to C API which insists that all function calls pass an associated 'isolate thread' which maps to the isolated the execution context for the function call.

libnjkafka does not allow C threads to share isolate threads.

The C main thread is attached to a isolate thread by default, further threads can create their own isolate threads by calling `libnjkafka_init_thread()` and clean up with `libnjkafka_teardown_thread()`.
