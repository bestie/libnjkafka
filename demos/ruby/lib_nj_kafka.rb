require "forwardable"
require File.join(ENV.fetch("C_EXT_PATH"), "libnjkafka_ext")

module LibNJKafka
  class Consumer
    def subscribe(topic, rebalance_listener: nil)
      cext_subscribe(topic, rebalance_listener)
    end
  end

  class TopicPartitionList
    def initialize(topic_partitions)
      @topic_partitions = topic_partitions
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
      other.is_a?(TopicPartition) &&
        other.topic == topic &&
        other.partition == partition
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
