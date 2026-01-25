#!/usr/bin/env bash

set -e

echo "Will install Apache Kafka Java library to `$KAFKA_HOME`"

kafka_dir="kafka_2.13-3.9.0"
kafka_archive="$kafka_dir.tgz"

wget "https://downloads.apache.org/kafka/3.9.0/${kafka_archive}"
mkdir -p ./lib
tar -xzf $kafka_archive
rm $kafka_archive
mv $kafka_dir $KAFKA_HOME
