package com.zendesk.code_gen;

import java.util.List;
import java.util.ArrayList;
import org.graalvm.nativeimage.c.struct.SizeOf;
import org.graalvm.word.PointerBase;

import com.zendesk.libnjkafka.Structs;

public class StructRegistryGenerator {

    public static void main(String[] args) {
        StructRegistryGenerator generator = new StructRegistryGenerator();
        generator.generateStructSizeRegistry();
    }

    private void generateStructSizeRegistry() {
        System.out.println(top);
        List<String> entries = mapEntryStrings();
        String indent = "    ".repeat(indentLevel);
        for (String entry : entries) {
            System.out.println(indent + entry);
        }
        System.out.println(bottom);
    }

    private String top = """
// Generated by StructRegistryGenerator
package com.zendesk.libnjkafka;

import java.util.Map;
import java.util.HashMap;

import org.graalvm.word.PointerBase;
import org.graalvm.word.WordFactory;
import org.graalvm.word.UnsignedWord;

public class StructSizeRegistry {
    private static final Map<Class<? extends PointerBase>, Integer> STRUCT_SIZES = new HashMap<>();

    static {
    """;

    private int indentLevel = 2;

    private String bottom = """
    }

    public static UnsignedWord get(Class<? extends PointerBase> structClass) {
        if (!STRUCT_SIZES.containsKey(structClass)) {
            throw new IllegalArgumentException("Size not defined for struct: " + structClass.getName());
        }
        return WordFactory.unsigned(STRUCT_SIZES.get(structClass));
    }
}
    """;


    public static List<String> mapEntryStrings() {
        List<String> entries = new ArrayList<>();
        String entry;
        Class<? extends PointerBase> struct;
        long size;

        struct = Structs.ConsumerConfigLayout.class;
        size = SizeOf.get(struct);
        entry = "STRUCT_SIZES.put(Structs." + struct.getSimpleName() + ".class, " + size + ");";
        entries.add(entry);
        System.err.println(entry);

        struct = Structs.ConsumerRecordLayout.class;
        size = SizeOf.get(struct);
        entry = "STRUCT_SIZES.put(Structs." + struct.getSimpleName() + ".class, " + size + ");";
        entries.add(entry);
        System.err.println(entry);

        struct = Structs.ConsumerRecordListLayout.class;
        size = SizeOf.get(struct);
        entry = "STRUCT_SIZES.put(Structs." + struct.getSimpleName() + ".class, " + size + ");";
        entries.add(entry);
        System.err.println(entry);

        struct = Structs.TopicPartitionLayout.class;
        size = SizeOf.get(struct);
        entry = "STRUCT_SIZES.put(Structs." + struct.getSimpleName() + ".class, " + size + ");";
        entries.add(entry);
        System.err.println(entry);

        struct = Structs.TopicPartitionListLayout.class;
        size = SizeOf.get(struct);
        entry = "STRUCT_SIZES.put(Structs." + struct.getSimpleName() + ".class, " + size + ");";
        entries.add(entry);
        System.err.println(entry);

        struct = Structs.TopicPartitionOffsetAndMetadataLayout.class;
        size = SizeOf.get(struct);
        entry = "STRUCT_SIZES.put(Structs." + struct.getSimpleName() + ".class, " + size + ");";
        entries.add(entry);
        System.err.println(entry);

        struct = Structs.TopicPartitionOffsetAndMetadataListLayout.class;
        size = SizeOf.get(struct);
        entry = "STRUCT_SIZES.put(Structs." + struct.getSimpleName() + ".class, " + size + ");";
        entries.add(entry);
        System.err.println(entry);

        struct = Structs.TopicPartitionOffsetAndMetadataLayout.class;
        size = SizeOf.get(struct);
        entry = "STRUCT_SIZES.put(Structs." + struct.getSimpleName() + ".class, " + size + ");";
        entries.add(entry);
        System.err.println(entry);

        return entries;
    }
}
