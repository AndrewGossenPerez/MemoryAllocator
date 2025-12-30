// block.hpp, created by Andrew Gossen.

#pragma once 
#include <cstddef> 
#include <cstdint>
#include <cstdio>

struct alignas(std::max_align_t) Block{

    std::size_t size; // The size of the block 
    // inclusive of the header, payload and the footer 
    bool allocated;
    // Double-linked list used when this block is in the free list i.e. unallocated 
    Block* prevFree;
    Block* nextFree;

};

struct alignas(std::max_align_t)  Footer{
    std::size_t size;
};

// --- Helpers

static constexpr std::size_t ALIGN=alignof(std::max_align_t);

static std::size_t minBlockSize() {
    // Get the minimum block size of the heap to validate bytes parameter for heap 
    return sizeof(Block) + ALIGN + sizeof(Footer);
}

static std::size_t alignMax(std::size_t x) {
    // Round up x to nearest multiple of the maximum allingment
    return (x + (ALIGN - 1)) & ~(ALIGN - 1);
}

static std::size_t getSize(std::size_t payloadBytes) {
    // REturns net required size for a block with size payloadBytes 
    payloadBytes = alignMax(payloadBytes);
    std::size_t total = sizeof(Block) + payloadBytes + sizeof(Footer);
    return alignMax(total); // important
}

static Footer* getFooter(Block* b) { // Gets the footer of Block pointer b 
    // Finds footer through pointer arithmetic deducting the header and payload
    return reinterpret_cast<Footer*>(
        reinterpret_cast<std::uint8_t*>(b) + b->size - sizeof(Footer)
    );
}

static void writeFooter(Block* b) { // Set up the footer of a Block b to have the same size 
    // The point of this is so we can easily access headers of prev blocks in O(1) time complexity 
    // by using pointer arithmetic and deducting by the block size
    getFooter(b)->size = b->size;
}