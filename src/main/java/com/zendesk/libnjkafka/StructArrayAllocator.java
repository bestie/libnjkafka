package com.zendesk.libnjkafka;

import java.util.function.BiConsumer;
import java.util.Iterator;
import java.util.List;

import org.graalvm.word.Pointer;
import org.graalvm.word.PointerBase;
import org.graalvm.nativeimage.UnmanagedMemory;

import org.graalvm.nativeimage.c.type.CCharPointer;
import org.graalvm.nativeimage.c.type.CTypeConversion;
import org.graalvm.word.UnsignedWord;
import org.graalvm.word.WordFactory;

import com.zendesk.libnjkafka.Structs.ArrayWrapper;

public class StructArrayAllocator<A> {
    // public static <A> Pointer allocate(
    //     List<A> objList,
    //     int arrayStructMemorySize,
    //     int structMemorySize,
    //     BiConsumer<A, StructArrayAllocator<A>> consumer
    // ) {
    //     StructArrayAllocator<A> allocator = new StructArrayAllocator<>(
    //         objList,
    //         arrayStructMemorySize,
    //         structMemorySize,
    //         consumer
    //     );

    //     allocator.allocate();
    //     allocator.populate();

    //     return null;
    // }

    private int itemCount;
    private int arrayStructMemorySize;
    private int structMemorySize;
    private BiConsumer<A, StructArrayAllocator<A>> consumer;
    private Pointer itemsStartPointer;
    private int index;
    private Iterator<A> objIter;
    private Pointer arrayStructPointer;
    public A currentItem;

    public StructArrayAllocator(Iterator<A> objIter, int iteratorCount, int arrayStructMemorySize, int structMemorySize, BiConsumer<A, StructArrayAllocator<A>> consumer) {
        this.objIter = objIter;
        this.itemCount = iteratorCount;
        this.arrayStructMemorySize = arrayStructMemorySize;
        this.structMemorySize = structMemorySize;
        this.consumer = consumer;

        this.index = -1;
    }

    public void allocate() {
        int totalMemorySize = arrayStructMemorySize + structMemorySize * this.itemCount;
        this.arrayStructPointer = UnmanagedMemory.calloc(totalMemorySize);
        ArrayWrapper arrayStruct = (ArrayWrapper) this.arrayStructPointer;
        this.itemsStartPointer = this.arrayStructPointer.add(arrayStructMemorySize);

        arrayStruct.setItems(this.itemsStartPointer);
        arrayStruct.setCount(0);   
    }

    public Pointer populate() {
        while (this.objIter.hasNext()) {
            this.currentItem = this.objIter.next();
            this.index = this.index + 1;

            this.consumer.accept(this.currentItem, this);
        }

        return this.arrayStructPointer;
    }

    public PointerBase currentStructPointer() {
        Pointer offset = this.itemsStartPointer.add(this.structMemorySize * this.index);
        return offset;
    }

    public PointerBase structPointer() {
        return this.currentStructPointer();
    }

    public CCharPointer cString(String javaString) {
        return CTypeConversion.toCString(javaString).get();
    }
}
