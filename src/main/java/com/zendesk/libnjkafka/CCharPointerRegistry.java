package com.zendesk.libnjkafka;

import java.util.HashMap;
import org.graalvm.nativeimage.c.type.CTypeConversion.CCharPointerHolder;

public class CCharPointerRegistry {
    private HashMap<Long, CCharPointerHolder> registry = new HashMap<>();

    public void put(long pointerAddress, CCharPointerHolder cStringHolder) {
        registry.put(pointerAddress, cStringHolder);
    }

    public CCharPointerHolder get(long pointerAddress) {
        return registry.get(pointerAddress);
    }

    public void remove(long pointerAddress) {
        registry.remove(pointerAddress);
    }

    public int size() {
        return registry.size();
    }
}