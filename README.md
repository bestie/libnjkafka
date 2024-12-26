# libnjkafka

CURRENTLY AN EXPERIMENTAL PROOF OF CONCEPT

libnjkafka is a "native Java" C compatible client library for Apache Kafka.

It provides a native, C compatible API on top of the official Apache Kafka client library.

It does not run on the JVM.

It starts up _fast_.

You can use it in your C programs, C extensions or via FFI.

## Why?

Kafka is major part of Zendesk infrastructure with a high level of complexity on the client side.

The Java library is the official, canonical, best maintained and best documented Kafka client available.

Replica libraries have shown to differ in behavior and default configuration (intentional or otherwise) which can be a problem.

Aligning on the same tech / libraries is better than diverging.

Alternatives exist in varying levels of maturity and feature completeness.

Even `librdkafka` lags behind and explicitly excludes features despite being supported by Confluent.

## C API

The public C API provides convenient functions:

```c
libnjkafka_create_consumer
libnjkafka_consumer_subscribe
libnjkafka_consumer_poll
libnjkafka_consumer_poll_each_message // automatically frees memory and commits offsets
libnjkafka_consumer_commit_all_sync
libnjkafka_consumer_close
```

## Ruby C extension

Included is a Ruby C extension and test program that runs under the latest MRI (3.3.3).

## Go

Seems easy.

## TypeScript

How hard could it be?

## Thread Safety

GraalVM provides some safety via its Java to C API which insists that all function calls pass an associated 'isolate thread' which maps to the isolated the execution context for the function call.

libnjkafka does not allow C threads to share isolate threads.

The C main thread is attached to a isolate thread by default, further threads can create their own isolate threads by calling `libnjkafka_init_thread()` and clean up with `libnjkafka_teardown_thread()`.

## Building the library

Instructions are for macOS hosts to build natively or using Docker.

There is a horrible Makefile that handles most of it.

### Build The Docker Image

```
make docker-build
```

To build the library you need a Kafka broker because the code analysis stage executes a simple consumer program.

The included Docker Compose configuration, will run a Kafka Broker and Zookeeper.

There must also be a topic with some messages to read. `make topic` create and populate a topic.
```
docker-compose up --detach
make topic docker-make
```
Artifacts will be written to your host's file system in the project's build directory.

### Build On macOS

You will need:
* GraalVM for building the native image from Java code
* clang for compiling the C API wrapper, C demo and Ruby C extension
* Apache Kafka Consumer Java library
* A Kakfa broker and Zookeeper server for code analysis

You must set `GRAALVM_HOME` and `KAFKA_HOME` to point to the relevent installations.

Assuming those are in place, you can build using the horrible Makefile.

Build the library:
```
make all
```

Run the C demo program:
```
make c_demo
```

Run the Ruby demo program:
```
make ruby_demo
```

#### Download GraalVM

In the project root:

```
mkdir -p graalvm
cd graalvm
wget https://download.oracle.com/graalvm/22/latest/graalvm-jdk-22_macos-x64_bin.tar.gz
tar -xzf graalvm-jdk-22_macos-x64_bin.tar.gz
```

Then set the environment variable, maybe in a `.env` file

```
export GRAALVM_HOME=./graalvm-jdk-22.0.1+8.1/Contents/Home
```

#### Download Kafka Consumer Library

In the project root:

```
mkdir -p lib
cd lib
wget https://downloads.apache.org/kafka/3.7.0/kafka_2.13-3.7.0.tgz
tar -xzf kafka_2.13-3.7.0.tgz
```

Then set the environment variable, maybe in a `.env` file

```
export KAFKA_HOME=./lib/kafka_2.13-3.7.0
```

## Copyright and license

Copyright 2024 Zendesk, Inc.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
