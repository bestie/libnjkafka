# libnjkafka

CURRENTLY AN EXPERIMENTAL PROOF OF CONCEPT

libnjkafka combines the offical Apache Kafka Java client, GraalVM's native Java
compilation capabilities and convenient C API wrapper.

It behaves like a regular C library.
It has no dependencies other than libc.
It does not run on the JVM.
It starts up _fast_.

## Why?

The official Java library is the best maintained and supported library available.

Alternative implementations vary in behvior, maturity and feature completeness.

It allows JVM and non-JVM projects to share consistent client features and behavior.

Finally, After experiencing some of the issues elluded to above, I decided this would be a fun pet project.

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
## Other Languages

Included is a Ruby C extension and test program that runs under the a recent MRI version (3.3.6).

Python, Go and many other languages can easily interface with libnjkafka via FFI.

## Thread Safety

While yet untested, libnjkafka will leverage GraalVM functionality to provide thread safety.

## Building the library

Instructions are for macOS hosts to build natively or using Docker.

There is a horrible Makefile that handles most of it.

### Build The Docker Image

```
make docker-build
```

To build the library you need a Kafka broker because the code analysis stage executes a simple consumer program.

The included Docker Compose configuration, will run a Kafka Broker and Zookeeper.

```
docker-compose up --detach
```

There must also be a topic with some messages to read. `make topic` creates and populates a topic with tests messages.

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
