// heap.hpp, created by Andrew Gossen.

#pragma once 
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <sys/mman.h>   
#include <unistd.h>    
#include "include/block.hpp" 
#include "include/priority.hpp"

// ---- Helper functions 

static std::size_t pageSize() {
    // Get the page-size for this system
    long ps = ::sysconf(_SC_PAGESIZE);
    if (ps <= 0) return 4096; 
    return (std::size_t)ps;
}

static std::size_t align(std::size_t x, std::size_t a) {
    // Round x to the nearest multiple of a 
    return (x + (a - 1)) & ~(a - 1);
}

static std::uintptr_t alignPtr(std::uintptr_t p, std::size_t a) {
    return (p + (a - 1)) & ~(std::uintptr_t)(a - 1);
}

// ---- RAII Heap structure 

class Heap{ 

public:

    // Main methods 
    void* alloc(std::size_t bytes,AllocationPriority priorityType);
    void release(void* ptr);

    explicit Heap(std::size_t bytes){

        if (bytes<minBlockSize()){
            m_size = 0;
            m_base = nullptr;
            m_freeHead = nullptr;
            m_end = nullptr;
            return;
        }

        std::size_t ps = pageSize(); // Get the size of each page for the current system
        bytes = align(bytes,ps); // Ensure that we align bytes to the page size 
        // This is so we ensure the entire heap has an integer amount of pages 

        // Reserve the new aligned bytes as virtual memory, using mmap directly 
        void* p = ::mmap(nullptr,bytes,PROT_READ | PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS,-1,0);
        
        if (p==MAP_FAILED){
            std::printf("mmap sys call failed: %s\n", std::strerror(errno));
            // Leave in a empty but valid state 
            m_size = 0;
            m_base = nullptr;
            m_freeHead = nullptr;
            m_end = nullptr;
            return;
        } 

        m_base = static_cast<std::uint8_t*>(p); // Cast untyped pointer p to a byte pointer so we can 
        // perform byte level pointer arithmetic, this is the base i.e. start of the heap ( first mem address )   
        m_size=bytes;
        m_end=m_base+m_size;

        // The whole heap will be one block at first, and it will split per allocation
        m_freeHead= reinterpret_cast<Block*>(m_base);
        m_freeHead->size= m_size;
        m_freeHead->allocated = false;
        m_freeHead->prevFree = nullptr;
        m_freeHead->nextFree = nullptr; 
        writeFooter(m_freeHead);

    };

    ~Heap() { // Free the mapping 
        if (m_base) {
            ::munmap(m_base, m_size);
        }
    }

    // Copying is disabled for heaps to prevent a double munmap
    Heap(const Heap&) = delete;
    Heap& operator=(const Heap&) = delete;

    Heap(Heap&& other) noexcept : m_size(other.m_size),m_base(other.m_base),
    m_end(other.m_end),m_freeHead(other.m_freeHead)
    {
        other.m_size=0;
        other.m_base=nullptr;
        other.m_end=nullptr;
        other.m_freeHead=nullptr;
    }

    Heap& operator=(Heap&& other) noexcept {
        if (this != &other) {
            if (m_base) ::munmap(m_base, m_size);

            m_size=other.m_size;
            m_base=other.m_base;
            m_end=other.m_end;
            m_freeHead=other.m_freeHead;

            other.m_size=0;
            other.m_base=nullptr;
            other.m_end=nullptr;
            other.m_freeHead=nullptr;
        }
        return *this;
    }

    // Getters 
    std::uint8_t* base() const { return m_base; }
    std::size_t size() const { return m_size; }
    bool active() const { return m_base != nullptr; } // Whether the Heap is currently active

    void printBlocks() const { // For debugging
        auto* p = m_base;
        while (p < m_end) {
            auto* b = reinterpret_cast<const Block*>(p);
            std::printf("Block %p size: %zu %s\n",(void*)b, b->size, b->allocated ? "ALLOCATED" : "FREE");
            p += b->size;
    }}

private:

    std::size_t m_size= 0;
    std::uint8_t* m_base = nullptr;
    std::uint8_t* m_end = nullptr;

    Block* m_freeHead = nullptr; // First item in the free list 

    void removeFree(Block* b){ // Set up the new prevFree and nextFree when a block is allocated
        if (b->prevFree) b->prevFree->nextFree=b->nextFree;
        else m_freeHead=b->nextFree; 
        if (b->nextFree) b->nextFree->prevFree=b->prevFree;
        b->prevFree=b->nextFree=nullptr; 
    }

    void pushFree(Block* b){ // When a block becomes free / unallocated
        b->allocated=false;
        b->prevFree=nullptr;
        b->nextFree=m_freeHead;
        if (m_freeHead) m_freeHead->prevFree=b;
        m_freeHead=b;
    }

};
