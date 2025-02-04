package com.zendesk.libnjkafka;

import java.util.Collection;
import java.util.HashSet;
import java.util.Set;

import org.apache.kafka.clients.consumer.ConsumerRebalanceListener;
import org.apache.kafka.common.TopicPartition;
import org.graalvm.nativeimage.IsolateThread;

import com.zendesk.libnjkafka.Structs.RebalanceListenerStruct;
import com.zendesk.libnjkafka.Structs.RebalanceCallback;
import com.zendesk.libnjkafka.Structs.TopicPartitionListLayout;

public class RebalanceCallbackAdapter implements ConsumerRebalanceListener {
	private RebalanceListenerStruct cCallbacks;
    private ConsumerProxy consumer;

    RebalanceCallbackAdapter(ConsumerProxy consumer, RebalanceListenerStruct cCallbacks) {
        this.consumer = consumer;
        this.cCallbacks = cCallbacks;
    }

    @Override
    public void onPartitionsAssigned(Collection<TopicPartition> partitions) {
        RebalanceCallback callback = cCallbacks.onPartitionsAssigned();
        handleEvent(partitions, callback);
    }

    @Override
    public void onPartitionsRevoked(Collection<TopicPartition> partitions) {
        RebalanceCallback callback = cCallbacks.onPartitionsRevoked();
        handleEvent(partitions, callback);
    }

    @Override
    public void onPartitionsLost(Collection<TopicPartition> partitions) {
        RebalanceCallback callback = cCallbacks.onPartitionsLost();
        handleEvent(partitions, callback);
    }


	private void handleEvent(Collection<TopicPartition> partitions, RebalanceCallback callback) {
		IsolateThread thread = consumer.getPollingThread();
        TopicPartitionListLayout cPartitions = convertToCStructs(partitions);

        if(thread.isNull()) {
            System.out.println("Thread is null");
            return;
        }

        if(callback.isNull()) {
            System.out.println("Callback is null");
            return;
        }

        callback.invoke(thread ,cPartitions);
	}

	private TopicPartitionListLayout convertToCStructs(Collection<TopicPartition> partitions) {
		Set<TopicPartition> partitionsSet = new HashSet<>(partitions);
        TopicPartitionListLayout cPartitions = Entrypoints.toCStruct(partitionsSet);
		return cPartitions;
	}
}
