
#include <iostream>
#include <chrono>
#include <thread>
#include "include/heap.hpp"

int main(){

    std::printf("-Allocator running-\n");
    Heap heap(1000); // Will auto round to nearest page number 

    std::printf("Starting Data : ");
    heap.printBlocks();
    std::printf("\n");

    void* ptr=heap.alloc(100,AllocationPriority::BestFit); // Allocate 100 bytes
    heap.printBlocks();
    
    std::printf("-- RELEASING --\n ");

    heap.release(ptr);
    heap.printBlocks();
    
    std::printf("-Program finished-");

    return 0; 

}