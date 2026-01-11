require 'securerandom'
require 'benchmark'
benchmarks = {}

benchmarks["Load C extension"] = Benchmark.realtime do
  require_relative "lib_nj_kafka"
end

kafka_topic = ENV.fetch('KAFKA_TOPIC')

partition_numbers = (0..11)
expected_record_count = 120
exit_code = 0

consumer = nil
group_id = "libnjkafka-ruby-demo-#{SecureRandom.uuid}"

class RebalanceListener
  def initialize
    @method_calls = []
  end

  attr_reader :method_calls

  def on_partitions_revoked(consumer, partitions)
    @method_calls << [:on_partitions_revoked, consumer, partitions]
    puts "Partitions revoked: #{partitions.inspect}"
  end

  def on_partitions_assigned(consumer, partitions)
    @method_calls << [:on_partitions_assigned, consumer, partitions]
    puts "Partitions assigned: #{partitions.inspect}"
  end
end
rebalance_listener = RebalanceListener.new

puts "Creating consumer with group.id=#{group_id}"

benchmarks["Create consumer"] = Benchmark.realtime do
  config = {
    auto_commit_interval_ms: 100,
    auto_offset_reset: "earliest",
    bootstrap_servers: ENV.fetch("KAFKA_BROKERS"),
    check_crcs: true,
    client_id: "libnjkafka_ruby_demo",
    enable_auto_commit: true,
    fetch_max_bytes: 1024 * 1024,
    fetch_max_wait_ms: 100,
    fetch_min_bytes: 1024 * 1024,
    group_id: group_id,
    heartbeat_interval_ms: 1000,
    isolation_level: "read_committed",
    max_partition_fetch_bytes: 1024 * 1024,
    max_poll_interval_ms: 1000,
    max_poll_records: 120,
    offset_reset: 'earliest',
    request_timeout_ms: 5000,
    session_timeout_ms: 6000,
  }

  consumer = LibNJKafka.create_consumer(config)
end

puts "Subscribing to #{kafka_topic}"
benchmarks["Subscribe to topic"] = Benchmark.realtime do
  consumer.subscribe(kafka_topic, rebalance_listener: rebalance_listener)
end

puts "Polling for messages (batch)"
offsets = []

if ENV["RUN_BATCH_POLL_RETURN"]
  benchmarks["Poll & Process (Batch return)"] = Benchmark.measure do
    records = consumer.poll(5000)
    records.each do |record|
      offsets << [record.partition, record.offset]
      print "."
    end
  end
else
  benchmarks["Poll & Process (each message block)"] = Benchmark.measure do
    consumer.poll_each_message(5000) do |record|
      offsets << [record.partition, record.offset]
      print "."
    end
  end
end

record_count = offsets.uniq.size

puts "Committing offsets synchronously"
benchmarks["Commit offsets"] = Benchmark.realtime do
  consumer.commit_all_sync(1000)
end

puts "Closing consumer"
benchmarks["Close consumer"] = Benchmark.realtime do
  consumer.close
end

GREEN = "\e[32m"
RED = "\e[31m"
ANSI_RESET = "\e[0m"
at_exit { print ANSI_RESET }

failed = false

if record_count == expected_record_count
  puts GREEN + "Got #{record_count}/#{expected_record_count} records."
else
  puts RED + "Got #{record_count}/#{expected_record_count} records."
  exit 1
end

all_topic_partitions = partition_numbers.map { |pn| LibNJKafka::TopicPartition.new(kafka_topic, pn) }

expected_rebalance_calls = [
  [:on_partitions_assigned, consumer.object_id, { kafka_topic => partition_numbers.to_a }],
  [:on_partitions_revoked, consumer.object_id, { kafka_topic => partition_numbers.to_a }],
]

actual_rebalance_method_calls = rebalance_listener.method_calls.map { |method_id, consumer, partitions|
  [method_id, consumer.object_id, partitions.to_h]
}

if actual_rebalance_method_calls == expected_rebalance_calls
  puts GREEN + "Got expected RebalanceListener method calls:"
else
  failed = true
  puts RED + "Did not receive expected rebalance listener method calls. \n" \
    "  Expected: \n"
  puts "  Got: "
end

puts ANSI_RESET
benchmarks.each do |name, time|
  ms = (time.real * 1000).round(4)
  puts "#{name}: \t#{ms}ms"
end

puts "Ruby version: #{RUBY_VERSION}"

exit_code = failed ? 1 : 0

exit exit_code
