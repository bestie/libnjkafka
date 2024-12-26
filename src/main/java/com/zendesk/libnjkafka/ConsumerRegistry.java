package src.main.java.com.zendesk.libnjkafka;

import java.util.HashMap;
import java.util.Map;
import java.util.Random;

public class ConsumerRegistry {
    private Map<Long, ConsumerProxy> consumers = new HashMap<>();
    private Random random = new Random();

    public long add(ConsumerProxy consumer) {
        long id = random.nextLong();
        consumers.put(id, consumer);
        return id;
    }

    public ConsumerProxy get(long id) {
        return consumers.get(id);
    }

    public void remove(long id) {
        consumers.remove(id);
    }
}
