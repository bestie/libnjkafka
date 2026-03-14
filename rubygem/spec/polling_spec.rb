require "securerandom"

RSpec.describe "Polling" do
  Thread.current.name = "rspec-main"

  let(:expected_record_count) { 120 }

  let(:partition_numbers) { (0..11) }
  let(:records_per_partition) { 10 }
  let(:group_id) { "libnjkafka-ruby-demo-#{SecureRandom.uuid}" }
  let(:kafka_topic) { ENV.fetch("KAFKA_TOPIC") }
  let(:rebalance_listener) { RebalanceListener.new }

  let(:config) do
    {
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
      max_poll_interval_ms: 10_000,
      max_poll_records: 120,
      offset_reset: "earliest",
      request_timeout_ms: 5000,
      session_timeout_ms: 6000
    }
  end

  it "polls all records and triggers rebalance callbacks 📨⚖️" do
    main_thread = Thread.current

    consumer = LibNJKafka.create_consumer(config)

    puts "📡 Subscribing to #{kafka_topic}"
    consumer.subscribe(kafka_topic, rebalance_listener: rebalance_listener)

    puts "🗳️ Polling records..."
    records = consumer.poll(5_000)

    processed_offsets = records.map { |r| [r.partition, r.offset] }

    records.each { print "." }
    puts "\n✅ Done processing"

    puts "💾 Committing offsets"
    consumer.commit_all_sync(1000)

    puts "🔒 Closing consumer"
    consumer.close

    # --- record checks ------------------------------------------------------

    expect(records.count).to eq(expected_record_count),
      "📨 Expected #{expected_record_count} records but got #{records.count}"

    expect(processed_offsets.count).to eq(expected_record_count)

    processed_offsets_by_partition =
      processed_offsets.group_by(&:first).transform_values { |v| v.map(&:last) }

    expected_offsets_by_partition =
      partition_numbers.to_h { |pn| [pn, (0..records_per_partition - 1).to_a] }

    expect(processed_offsets_by_partition).to eq(expected_offsets_by_partition),
      "📨 Missing offsets. Got: #{processed_offsets_by_partition.inspect}"

    # --- rebalance checks ---------------------------------------------------

    expected_topic_partition_list =
      LibNJKafka::TopicPartitionList.from_name_and_numbers(
        kafka_topic,
        partition_numbers
      )

    assignment_call = [
      main_thread,
      :on_partitions_assigned,
      consumer,
      expected_topic_partition_list
    ]

    revocation_call = [
      main_thread,
      :on_partitions_revoked,
      consumer,
      expected_topic_partition_list
    ]

    puts "⚖️➕ Checking assignment callback"
    expect(rebalance_listener.method_calls.first).to eq(assignment_call)

    puts "⚖️➖ Checking revocation callback"
    expect(rebalance_listener.method_calls.last).to eq(revocation_call)

    puts "🎉 Ruby #{RUBY_VERSION} test passed 🧑‍🍳💋"
  end

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
end
