require 'securerandom'
require 'benchmark'
benchmarks = {}

benchmarks["Load C extension"] = Benchmark.realtime do
  require_relative File.join(ENV.fetch("C_EXT_PATH"), "libnjkafka_ext")
end

expected_record_count = 120
exit_code = 0

consumer = nil
group_id = "libnjkafka-ruby-demo-#{SecureRandom.uuid}"

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

puts "Subscribing to #{ENV.fetch('KAFKA_TOPIC')}"
benchmarks["Subscribe to topic"] = Benchmark.realtime do
  consumer.subscribe(ENV.fetch('KAFKA_TOPIC'))
end

puts "Polling for messages"
record_count = 0
output = []
benchmarks["Poll / Consume"] = Benchmark.measure do
  consumer.poll_each_message(2000) do |record|
    record_count += 1
    output << record.inspect
  end
end

puts output

puts "Committing offsets synchronously"
benchmarks["Commit offsets"] = Benchmark.realtime do
  consumer.commit_all_sync(1000)
end

puts "Closing consumer"
benchmarks["Close consumer"] = Benchmark.realtime do
  consumer.close
end

ANSI_GREEN = "\e[32m"
ANSI_RED = "\e[31m"
ANSI_RESET = "\e[0m"
at_exit { print ANSI_RESET }

if record_count == expected_record_count
  print ANSI_GREEN
else
  print ANSI_RED
  exit_code = 1
end

puts "Got #{record_count}/#{expected_record_count} records."
benchmarks.each do |name, time|
  ms = (time.real * 1000).round(4)
  puts "#{name}: \t#{ms}ms"
end

puts "Ruby version: #{RUBY_VERSION}"

exit exit_code
