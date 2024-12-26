#!/bin/bash

if [ -z "$KAFKA_TOPIC" ]; then
  env | grep KAFKA
  echo "KAFKA_TOPIC is not set. Please set it to continue.";
  exit 1;
fi

if [ -z "$KAFKA_BROKERS" ]; then
  echo "KAFKA_BROKERS is not set. Please set it to continue.";
  exit 1;
fi
if [ -z "$KAFKA_HOME" ]; then
  echo "KAFKA_BROKERS is not set. Please set it to continue.";
  exit 1;
fi

message_count=120
partition_count=12
published_message_count=0

for ((i=1; i<=message_count; i++))
do
  key=$((i % partition_count))
  value="Message $i with key $key"
  published_message_count=$i
  echo "$key:$value"
done | "kafka-console-producer.sh" --broker-list "${KAFKA_BROKERS}" --topic "${KAFKA_TOPIC}" --property "parse.key=true" --property "key.separator=:"

echo "Published ${published_message_count} messages to ${KAFKA_TOPIC}."

kafka-console-consumer.sh --bootstrap-server ${KAFKA_BROKERS} --topic ${KAFKA_TOPIC} --from-beginning --max-messages $message_count --property print.key=true --property key.separator=":" | cat -n
