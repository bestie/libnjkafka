# libnjkafka

CURRENTLY AN EXPERIMENTAL PROOF OF CONCEPT

libnjkafka is a "native Java" Kafka client.

The official client is compiled to a native binary with GraalVM and wrapped in
a more convenient C API.

* It behaves like a regular C library.
* It runs natively with no JVM.
* It has no dependencies other than libc.
* It starts up _fast_.

## Why?

1. Alternative client implementations vary in behvior, maturity and feature completeness,
libnjkafka aims to provide a C compatible interface to the canonical implementation.
2. "It will be fun", they said.

## C API

The public C API provides convenient functions:

```c
libnjkafka_create_consumer
libnjkafka_consumer_subscribe
libnjkafka_consumer_poll
libnjkafka_consumer_poll_each_message // passes messages to a function then calls `free`
libnjkafka_consumer_commit_all_sync
libnjkafka_consumer_close
```
## Other Languages

Included is a Ruby C extension and test program that

Python, Go and many other languages can easily interface with libnjkafka via FFI.

## Building the library

The `Makefile` can build the library for macOS and Linux.

To build with Docker `make docker-make` will build the container, mount the
project directory and run `make` inside it.

The binary and headers will be written to the `./build` directory

A Kafka broker is necessary for the build process so GraalVM can run the Java
code and discover what Java code must be compiled into the native binary.

Both the `Makefile` and `Dockerfile` assume a Kafka broker is running on `localhost:9092`.
You can set the `KAFKA_BROKERS` environment variable to point to another host/port.
There is an included `Docker Compose` configuration to run a broker on the expected port (this how it builds in GitHub Actions).

## Copyright and license

Copyright 2024 Zendesk, Inc.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
