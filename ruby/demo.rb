require 'securerandom'
require 'benchmark'
require_relative File.join(ENV.fetch("C_EXT_PATH"), "libnjkafka_ext")

config = {
  group_id: "test-group-#{SecureRandom.uuid}",
  auto_commit_interval_ms: 1000,
  auto_offset_reset: 0,
  bootstrap_servers: ENV.fetch('KAFKA_BROKERS'),
  check_crcs: true,
  client_id: "libnjkafka_ruby_demo",
  enable_auto_commit: true,
  fetch_max_bytes: 1024 * 1024,
  fetch_max_wait_ms: 100,
  fetch_min_bytes: 1024 * 1024,
  heartbeat_interval: 1000,
  isolation_level: 0,
  max_partition_fetch_bytes: 1024 * 1024,
  max_poll_interval: 1000,
  max_poll_records: 100,
  max_poll_records: 100,
  offset_reset: 'earliest',
  request_timeout: 5000,
  session_timeout: 6000,
}

consumer = LibNJKafka.create_consumer(config)

begin
  consumer.subscribe(ENV.fetch('KAFKA_TOPIC'))
rescue => e
  puts e 
end

puts "Subscribed to #{ENV.fetch('KAFKA_TOPIC')}"
puts "about to poll"

records = nil
output = []
benchmark = Benchmark.measure do
  records = consumer.poll(100)
  records.each do |record|
    output << record.inspect
  end
end

consumer.close
puts output

ANSI_GREEN = "\e[32m"
ANSI_RED = "\e[31m"
ANSI_RESET = "\e[0m"
at_exit { print ANSI_RESET }

expected_record_count = 120

if records.count == expected_record_count
  print ANSI_GREEN
else
  print ANSI_RED
end

puts "Got #{records.size}/#{expected_record_count} records."
puts "Benchmark: #{benchmark}"
