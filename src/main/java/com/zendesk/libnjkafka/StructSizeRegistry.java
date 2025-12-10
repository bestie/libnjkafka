package com.zendesk.libnjkafka;

import java.util.Map;
import java.util.HashMap;

import org.graalvm.word.PointerBase;
import org.graalvm.word.WordFactory;
import org.graalvm.word.UnsignedWord;
import org.graalvm.nativeimage.c.struct.SizeOf;

public class StructSizeRegistry {
    private static final Map<Class<? extends PointerBase>, Integer> STRUCT_SIZES = new HashMap<>();

    static {
        STRUCT_SIZES.put(Structs.ConsumerConfigLayout.class, SizeOf.get(Structs.ConsumerConfigLayout.class));
        STRUCT_SIZES.put(Structs.ConsumerRecordLayout.class, SizeOf.get(Structs.ConsumerRecordLayout.class));
        STRUCT_SIZES.put(Structs.ConsumerRecordListLayout.class, SizeOf.get(Structs.ConsumerRecordListLayout.class));
        STRUCT_SIZES.put(Structs.TopicPartitionLayout.class, SizeOf.get(Structs.TopicPartitionLayout.class));
        STRUCT_SIZES.put(Structs.TopicPartitionListLayout.class, SizeOf.get(Structs.TopicPartitionListLayout.class));
        STRUCT_SIZES.put(Structs.TopicPartitionOffsetAndMetadataLayout.class, SizeOf.get(Structs.TopicPartitionOffsetAndMetadataLayout.class));
        STRUCT_SIZES.put(Structs.TopicPartitionOffsetAndMetadataListLayout.class, SizeOf.get(Structs.TopicPartitionOffsetAndMetadataListLayout.class));
        STRUCT_SIZES.put(Structs.TopicPartitionOffsetAndMetadataLayout.class, SizeOf.get(Structs.TopicPartitionOffsetAndMetadataLayout.class));
    }

    public static UnsignedWord get(Class<? extends PointerBase> structClass) {
        if (!STRUCT_SIZES.containsKey(structClass)) {
            throw new IllegalArgumentException("Size not defined for struct: " + structClass.getName());
        }
        return WordFactory.unsigned(STRUCT_SIZES.get(structClass));
    }
}

