require "forwardable"

require_relative "../ext/libnjkafka_ext"

module LibNJKafka
  module_function
  def ensure_current_thread_teardown
    return if Thread.current == Thread.main
    return if Thread.current[:__LibNJKafka_tracer]

    thread = Thread.current

    tracer = TracePoint.new(:thread_end) {
      teardown_current_thread
    }

    thread.thread_variable_set(:__LibNJKafka_tracer, tracer)
    tracer.enable(target_thread: thread)

    at_exit do
      if thread.alive?
        warn "LibNJKafka: thread found alive at exit. Gracefully shutdown threads that have executed LibNJKafka code to avoid issues. thread=#{thread.inspect}"
        thread.terminate
        thread.join
      end
    end
  end

  class Consumer
    def subscribe(topic, rebalance_listener: nil)
      cext_subscribe(topic, rebalance_listener)
    end
  end

  class TopicPartitionList
    class << self
      def from_name_and_numbers(name, numbers)
        new(numbers.map { |n| TopicPartition.new(name, n) })
      end
    end

    def initialize(topic_partitions)
      @topic_partitions = topic_partitions
    end
    attr_reader :topic_partitions

    def ==(other)
      other.topic_partitions.sort == topic_partitions.sort
    end

    def to_a
      @topic_partitions
    end

    def to_h
      @topic_partitions
        .group_by(&:topic)
        .to_h { |topic, tps|
          [topic, tps.map(&:partition).sort]
        }
    end
  end

  class TopicPartition
    def initialize(topic, partition)
      @topic = topic
      @partition = partition
    end

    attr_reader :topic, :partition

    def ==(other)
      other.topic == topic &&
        other.partition == partition
    end

    def <=>(other)
      [other.topic, other.partition] <=> [topic, partition]
    end
  end

  class ConsumerRecordList
    extend Forwardable
    include Enumerable

    def_delegators :@records, :each, :empty?, :size

    def initialize(records)
      @records = records
    end
    attr_reader :records

    def to_a
      @records
    end
  end

  class ConsumerRecord
    def initialize(
      headers: nil,
      key: nil,
      leader_epoch: nil,
      offset: nil,
      partition: nil,
      timestamp: nil,
      topic: nil,
      value: nil
    )
      @headers = headers
      @key = key
      @leader_epoch = leader_epoch
      @offset = offset
      @partition = partition
      @timestamp = timestamp
      @topic = topic
      @value = value
    end

    attr_reader \
      :headers,
      :key,
      :leader_epoch,
      :offset,
      :partition,
      :timestamp,
      :topic,
      :value
  end
end
