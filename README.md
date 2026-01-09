

This project implements an OS-backed memory allocator that creates my own heap using virtual memory through the mmap system call.

The custom heap is managed with RAII. The constructor maps the requested heap size (which is rounded up to a page-aligned size), and the destructor unmaps it with munmap.

The heap consists of blocks, following this architecture : Block Header - Payload - Block Footer. Where header stores the block's metadata, and footer stores the block's size to easily access headers in O(1) time complexity. 

---
**Benchmark results against the standard malloc/free functions are as follows:**

---

 FIRST-FIT ( 10,000 iterations with 10,000 operations )
 ---

 
 Immediate Allocation/Deallocation (Throughput) - 1.719x FASTER than malloc/free
 

 Allocation/Deallocation with fragmentation present - 2.2x FASTER than malloc/free 


 
 BEST-FIT ( 10,000 iterations with 1,000 operations ) 
---
 
 Immediate Allocation/Deallocation (Throughput) - 1.6x FASTER than malloc/free
 
 Allocation/Deallocation with fragmentation present - 4.1x SLOWER than malloc/free 




 

