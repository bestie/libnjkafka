#!/bin/bash

TOPIC_NAME="test-topic"
BROKER_LIST="localhost:9092"
message_count=120
partition_count=12
published_message_count=0

for ((i=1; i<=message_count; i++))
do
  key=$((i % partition_count))
  value="Message $i with key $key"
  published_message_count=$i
  echo "$key:$value"
done | "${KAFKA_HOME}/bin/kafka-console-producer.sh" --broker-list "${BROKER_LIST}" --topic "${TOPIC_NAME}" --property "parse.key=true" --property "key.separator=:"

echo "Published ${published_message_count} messages to ${TOPIC_NAME}."

${KAFKA_HOMe}/bin/kafka-console-consumer.sh --bootstrap-server ${BROKER_LIST} --topic ${TOPIC_NAME} --from-beginning --max-messages $message_count --property print.key=true --property key.separator=":" | cat -n
