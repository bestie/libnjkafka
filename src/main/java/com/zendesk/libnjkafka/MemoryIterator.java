package com.zendesk.libnjkafka;

import org.graalvm.word.Pointer;
import org.graalvm.word.PointerBase;
import org.graalvm.word.UnsignedWord;

import java.util.function.Consumer;

import org.graalvm.word.WordFactory;
import org.graalvm.nativeimage.UnmanagedMemory;
import org.graalvm.nativeimage.c.type.CCharPointer;
import org.graalvm.nativeimage.c.type.CTypeConversion;
import org.graalvm.nativeimage.c.type.CTypeConversion.CCharPointerHolder;

import com.zendesk.libnjkafka.Structs.ArrayWrapper;

public class MemoryIterator<A extends PointerBase, I extends PointerBase> {
    private int i;
    private int itemCount;
    private UnsignedWord arrayStructSize;
    private UnsignedWord structSize;
    private Pointer itemsPointer;
    private A arrayStructPointer;
    public static final CCharPointerRegistry stringRegistry = new CCharPointerRegistry();

    public static <A extends PointerBase, I extends PointerBase> A allocateAndPopulateStructArray(
            int itemCount, Class<A> arrayStructClass, Class<I> itemStructClass,
            Consumer<MemoryIterator<A, I>> populator) {
        MemoryIterator<A, I> iterator = new MemoryIterator<>(itemCount, arrayStructClass, itemStructClass);
        populator.accept(iterator);
        return iterator.finalizeArrayStruct();
    }

    public MemoryIterator(int itemCount, Class<A> arrayStructClass, Class<I> itemStructClass) {
        this.itemCount = itemCount;
        this.i = 0;
        this.structSize = StructSizeRegistry.get(itemStructClass);
        this.arrayStructSize = StructSizeRegistry.get(arrayStructClass);
        allocateMemory();
    }

    public boolean hasNext() {
        return i < this.itemCount;
    }

    @SuppressWarnings("unchecked")
    public I next() {
        if (i >= this.itemCount) {
            throw new IllegalStateException("MemoryIterator Error: Attempt to overflow allocated memory. Memory for " + itemCount + " was allocated.");
        }

        UnsignedWord offset = this.structSize.multiply(i);
        I currentPointer = (I) this.itemsPointer.add(offset);
        i++;

        return currentPointer;
    }

    private void ensureFullAllocation() {
        if (i != this.itemCount) {
            throw new IllegalStateException("MemoryIterator was not fully consumed");
        }
    }

    @SuppressWarnings("unchecked")
    private void allocateMemory() {
        UnsignedWord totalMemorySize = this.arrayStructSize.add(this.structSize.multiply(this.itemCount));
        System.out.println("  ✨✨✨ GraalVM allocating memory for struct array: itemCount=" + this.itemCount + ", totalMemorySize=" + totalMemorySize.rawValue());

        Pointer chunk = UnmanagedMemory.calloc(totalMemorySize);

        this.arrayStructPointer = (A) chunk;

        if(this.itemCount == 0) {
            //System.out.println("MemoryIterator: itemCount is 0, setting itemsPointer to null");
            this.itemsPointer = WordFactory.nullPointer();
        } else {
         this.itemsPointer = chunk.add(this.arrayStructSize);
        }
    }

    public CCharPointer cString(String javaString) {
        CCharPointerHolder cStringHolder = CTypeConversion.toCString(javaString);
        MemoryIterator.stringRegistry.put(this.arrayStructPointer.rawValue(), cStringHolder);
        CCharPointer cString = cStringHolder.get();
        return cString;
    }

    public A finalizeArrayStruct() {
        ensureFullAllocation();

        ArrayWrapper wrapper = (ArrayWrapper) this.arrayStructPointer;
        wrapper.setCount(this.itemCount);
        wrapper.setItems(this.itemsPointer);

        return this.arrayStructPointer;
    }
}
