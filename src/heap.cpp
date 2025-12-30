#include "include/heap.hpp"
#include <cstddef>

void* Heap::alloc(std::size_t bytes,AllocationPriority priorityType){

    // Allocates bytes amount of the heap, auto aligned       
                              
    if (bytes == 0 || !m_base) return nullptr;
    std::size_t requiredBytes= getSize(bytes);

    Block* current=nullptr;

    if (priorityType==AllocationPriority::FirstFit){
        
        // Apply first fit approach, find the first block with a sufficent size 
        // Best case O(1)
        current=m_freeHead;
        while (current && current->size<requiredBytes){ // Go through each block in the free list
            current=current->nextFree;
        }
        
    } else if (priorityType==AllocationPriority::BestFit){

        // Always going to be O(n) since it will have to search through entire free list 
        
        for (Block* b=m_freeHead;b;b=b->nextFree){
            // Go through each block in the free list 
            if (b->size>=requiredBytes){
                if (!current || current->size > b->size) current=b;
            }
        }

    }

    if (!current){ // No sufficent block found, i.e. full heap 
        std::printf("alloc() Error: No sufficent space in the heap\n"); 
        return nullptr;
    } 

    removeFree(current); // Prepare to allocate 

    // Split the block if possible 
    const std::size_t rem =current->size-requiredBytes;

    if (rem>=minBlockSize()){ // Remainding space is sufficent to create a new block 

        auto* remainder = reinterpret_cast<Block*>( // Use byte arithmetic to go to the required boundary of the block
            // Split up any memory after this point into a seperate block  
            reinterpret_cast<std::uint8_t*>(current) + requiredBytes
        ); 

        current->size=requiredBytes;
        remainder->size=rem;
        remainder->allocated=false;
        remainder->prevFree=nullptr;
        remainder->nextFree=nullptr;

        writeFooter(remainder); // Mark the end of the new freely made block
        pushFree(remainder);

    } else { 
        // Otherwise can't split so full block will be allocated, which is already done 
    } 

    current->allocated = true;
    writeFooter(current); // Mark end of the allocated block 
    return reinterpret_cast<std::uint8_t*>(current) + sizeof(Block); // return pointer to the payload

};

void Heap::release(void* ptr){

    // ptr should be a pointer to the payload of a Block 

    if (!ptr) return;
    if (!m_base) return;

    auto* b = reinterpret_cast<Block*>( // Locate the footer of the block
        reinterpret_cast<std::uint8_t*>(ptr) - sizeof(Block)
    );

    // Range safeguard 
    if (reinterpret_cast<std::uint8_t*>(b) < m_base || reinterpret_cast<std::uint8_t*>(b) >= m_end) {
        std::printf("release() Error: The pointer is out of heap range\n");
        return;
    }

    if (!b->allocated) { // Avoid double free 
        std::printf("release() Error: This memory has already been freed\n");
        return;
    }

    b->allocated = false;

    // Attempt to coaelasce with next block, i.e. merge this newly freed block with next block 
    Block* next = nullptr;

    { // Attempt to merge with next block 
        auto* nextBlock = reinterpret_cast<std::uint8_t*>(b)+ b->size;
        if (nextBlock < m_end) {
            next = reinterpret_cast<Block*>(nextBlock);
            if (!next->allocated) {

                // Remove the next block to merge 
                removeFree(next);
                //Merge b with this next block 
                b->size+=next->size;
                writeFooter(b); // Mark the end of b now that size has been adjusted 

            }
        }
    }

    { // Attempt to coaelasce with previous block

        auto* base=reinterpret_cast<std::uint8_t*>(b);

        if (base>m_base){ // The footer of the previous block is right before the header of Block b
            // as b is not the first free block in the free list 

            auto* prevFooter=reinterpret_cast<Footer*>(base-sizeof(Footer));
            std::size_t prevSize = prevFooter->size;
            auto* prevBase = base-prevSize;

            if (prevBase>=m_base){ // Ensure the previous block falls within heap range 
                Block* prev=reinterpret_cast<Block*>(prev);
                if (!prev->allocated && prev->size==prevSize ){// Ensure unallocated and size is unchanged 
                    removeFree(prev);
                    prev->size += b->size;
                    writeFooter(prev); // Mark the end of the new merged prev
                    b=prev; // THe new free block is now prev as it's merged with b 
                } 

            }

        } 

        pushFree(b); // Add BLock b to the free list 

    }

}