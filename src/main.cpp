
#include <iostream>
#include "include/heap.hpp"


int main(){

    std::cout << " Allocator running " << std::endl;
    Heap heap(1000); // Will auto round to nearest page number 
    void* ptr=heap.alloc(100,AllocationPriority::FirstFit); // Allocate 100 byte 
    heap.release(ptr);
    heap.printBlocks();
    
    return 0; 

}