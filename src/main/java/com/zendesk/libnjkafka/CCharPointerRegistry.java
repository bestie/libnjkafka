package com.zendesk.libnjkafka;

import java.util.HashMap;
import java.util.List;

import org.graalvm.nativeimage.c.type.CTypeConversion.CCharPointerHolder;

public class CCharPointerRegistry {
    private HashMap<Long, List<CCharPointerHolder>> map;

    public CCharPointerRegistry() {
        this.map = new HashMap<>();
    }

    public void put(long pointerAddress, List<CCharPointerHolder> cStringHolders) {
        map.put(pointerAddress, cStringHolders);
    }

    public List<CCharPointerHolder> get(long pointerAddress) {
        return map.get(pointerAddress);
    }

    public void remove(long pointerAddress) {
        map.remove(pointerAddress);
    }

    public int size() {
        return map.size();
    }

    public int sumOfCounts() {
        int sum = 0;
        for (List<CCharPointerHolder> holders : map.values()) {
            sum += holders.size();
        }
        return sum;
    }
}