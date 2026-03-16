// block.hpp, created by Andrew Gossen.

#pragma once 
#include <cstddef> 
#include <cstdint>
#include <cstdio>

struct alignas(std::max_align_t) Block{

    std::size_t size; // Total block size, header + payload + footer.
    bool allocated;
    // Doubly linked free-list pointers, used only when the block is free
    Block* prevFree;
    Block* nextFree;

};

struct alignas(std::max_align_t) Footer{
    std::size_t size;
};

// --- Helpers

inline constexpr std::size_t ALIGN=alignof(std::max_align_t);

inline std::size_t minBlockSize() {
    // Minimum usable block size, header + minimum aligned payload + footer
    return sizeof(Block) + ALIGN + sizeof(Footer);
}

inline std::size_t alignMax(std::size_t x) {
    // Round x up to the next multiple of the maximum alignment
    return (x + (ALIGN - 1)) & ~(ALIGN - 1);
}

inline std::size_t getSize(std::size_t payloadBytes) {
    // Return the total block size required for a payload of payloadBytes,
    // including header, footer, and alignment padding.
    payloadBytes = alignMax(payloadBytes);
    std::size_t total = sizeof(Block) + payloadBytes + sizeof(Footer);
    return alignMax(total); // important
}

inline Footer* getFooter(Block* b) { 
    // Return a pointer to the footer at the end of block b.
    return reinterpret_cast<Footer*>(
        reinterpret_cast<std::uint8_t*>(b) + b->size - sizeof(Footer)
    );
}

inline void writeFooter(Block* b) { 
    // Mirror the block size into the footer so the previous block can be
    // found in O(1) during coalescing.
    getFooter(b)->size = b->size;
}