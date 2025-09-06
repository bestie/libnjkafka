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
end
