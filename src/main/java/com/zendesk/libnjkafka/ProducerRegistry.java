package com.zendesk.libnjkafka;

import java.util.HashMap;
import java.util.Map;
import java.util.Random;

public class ProducerRegistry {
    private Map<Long, ProducerProxy> consumers = new HashMap<>();
    private Random random = new Random();

    public long add(ProducerProxy consumer) {
        long id = random.nextLong();
        consumers.put(id, consumer);
        return id;
    }

    public ProducerProxy get(long id) {
        return consumers.get(id);
    }

    public void remove(long id) {
        consumers.remove(id);
    }
}
