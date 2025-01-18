#!/usr/bin/env bash

set -e

wget https://download.oracle.com/graalvm/23/latest/graalvm-jdk-23_macos-aarch64_bin.tar.gz
tar -xzf graalvm-jdk-23_macos-aarch64_bin.tar.gz
sudo mv graalvm-jdk-23.0.1+11.1 /Library/Java/JavaVirtualMachines/
rm graalvm-jdk-23_macos-aarch64_bin.tar.gz

touch .env
echo "export GRAALVM_HOME=/Library/Java/JavaVirtualMachines/graalvm-jdk-23.0.1+11.1/Contents/Home/" >> .env
echo "export JAVA_HOME=$GRAALVM_HOME" >> .env

KAFKA_HOME="$PWD/lib/kafka"
kafka_dir="kafka_2.13-3.9.0"
kafka_archive="$kafka_dir.tgz"

wget "https://downloads.apache.org/kafka/3.9.0/${kafka_archive}"
mkdir -p ./lib
tar -xzf $kafka_archive
rm $kafka_archive
mv $kafka_dir $KAFKA_HOME
