package com.zendesk.libnjkafka;

import java.util.Set;
import java.util.HashSet;
import java.util.concurrent.ConcurrentHashMap;

public class PointerGraph {
    private ConcurrentHashMap<Long, HashSet<Long>> graph;

    public PointerGraph() {
        this.graph = new ConcurrentHashMap<>();
    }

    public void addNode(Long root) {
        graph.put(root, emptyValue());
    }

    public void addDependency(Long node, Long dependent) {
        graph.putIfAbsent(dependent, emptyValue());
        graph.get(node).add(dependent);
    }

    public Set<Long> getDependents(Long parent) {
        return graph.get(parent);
    }

    public void remove(Long node) {
        graph.remove(node);
    }

    private HashSet<Long> emptyValue() {
        return new HashSet<>();
    }
}
