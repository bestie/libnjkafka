require 'securerandom'

require_relative "lib_nj_kafka"

Thread.current.name = "main"

consumer = nil
exit_code = 0
expected_record_count = 120
failed = false
group_id = "libnjkafka-ruby-demo-#{SecureRandom.uuid}"
kafka_topic = ENV.fetch('KAFKA_TOPIC')
main_thread = Thread.current
partition_numbers = (0..11)
records_per_partition = 10

class RebalanceListener
  def initialize
    @method_calls = []
  end

  attr_reader :method_calls

  def on_partitions_assigned(consumer, partitions)
    @method_calls << [Thread.current, :on_partitions_assigned, consumer, partitions]
  end

  def on_partitions_revoked(consumer, partitions)
    @method_calls << [Thread.current, :on_partitions_revoked, consumer, partitions]
  end
end
rebalance_listener = RebalanceListener.new

puts "Creating consumer with group.id=#{group_id}"

config = {
  auto_offset_reset: "earliest",
  bootstrap_servers: ENV.fetch("KAFKA_BROKERS"),
  check_crcs: true,
  client_id: "libnjkafka_ruby_demo",
  enable_auto_commit: false,
  fetch_max_bytes: 1024 * 1024,
  fetch_max_wait_ms: 100,
  fetch_min_bytes: 1024 * 1024,
  group_id: group_id,
  heartbeat_interval_ms: 1000,
  isolation_level: "read_committed",
  max_partition_fetch_bytes: 1024 * 1024,
  max_poll_interval_ms: 10000,
  max_poll_records: 120,
  offset_reset: 'earliest',
  request_timeout_ms: 5000,
  session_timeout_ms: 6000,
}

consumer = LibNJKafka.create_consumer(config)

puts "Subscribing to #{kafka_topic}"
consumer.subscribe(kafka_topic, rebalance_listener: rebalance_listener)

records = []

puts "Polling"
poll_timeout_ms = 5_000

records = consumer.poll(poll_timeout_ms)

record_count = records.count

processed_offsets = []
records.each do |record|
  processed_offsets << [record.partition, record.offset]
  print "."
end
puts "Done processing"

puts "Committing offsets synchronously"
consumer.commit_all_sync(1000)

puts "Closing consumer"
consumer.close

GREEN = "\e[32m"
RED = "\e[31m"
ANSI_RESET = "\e[0m"
at_exit { print ANSI_RESET }

puts "📨 Expecting to have polled and processed #{expected_record_count} records."
if record_count == expected_record_count && processed_offsets.count == expected_record_count
  puts GREEN + "  Got #{record_count}/#{expected_record_count} records."
else
  puts RED + "  Got #{record_count}/#{expected_record_count} records."
end
puts ANSI_RESET

puts "📨 Expecting to have recevied all offsets for each partition"
processed_offsets_by_partition = processed_offsets.group_by(&:first).transform_values { |v| v.map(&:last) }
expected_offsets_by_partition = partition_numbers.to_h { |pn|
  [pn, (0..records_per_partition-1).to_a]
}

if expected_offsets_by_partition == processed_offsets_by_partition
  puts GREEN + "    All offsets on all partitions present"
else
  puts RED + "    Missing offsets. Got: #{processed_offsets_by_partition.inspect}"
end
puts ANSI_RESET

expected_topic_partition_list = LibNJKafka::TopicPartitionList.from_name_and_numbers(kafka_topic, partition_numbers)

# Partitions assigned event will execute in worker thread where the consumer is polling
assignment_callback_called = rebalance_listener.method_calls.first == [
  main_thread,
  :on_partitions_assigned,
  consumer,
  expected_topic_partition_list
]

# Partitions revoked event will fire in main thread where consumer is closed
revokation_callback_called = rebalance_listener.method_calls.last == [
  main_thread,
  :on_partitions_revoked,
  consumer,
  expected_topic_partition_list
]

puts "⚖️➕ Expecting the rebalance listener to have been notified of partitions assigned"
if assignment_callback_called
  puts GREEN + "    Got rebalance listener on_partitions_assigned"
else
  failed = true
  puts RED + "    Did not get rebalance listener on_partitions_assigned"
  puts "  Methods calls received: #{rebalance_listener.method_calls.inspect}"
end
puts ANSI_RESET

puts "⚖️➖ Expecting the rebalance listener to have been notified of partitions assigned"
if revokation_callback_called
  puts GREEN + "  Got rebalance listener on_partitions_revoked"
else
  failed = true
  puts RED + "  Did not get rebalance listener on_partitions_revoked"
  puts "  Methods calls received: #{rebalance_listener.method_calls.inspect}"
end
puts ANSI_RESET

puts "Ruby version: #{RUBY_VERSION}"

exit_code = failed ? 1 : 0

if exit_code == 0
  puts GREEN + "Passed 🧑‍🍳💋  "
else
  puts RED + "Failed 👎"
end
puts ANSI_RESET

exit exit_code
