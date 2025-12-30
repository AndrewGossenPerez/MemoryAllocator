
<img width="200" height="200" alt="MemIcon" src="https://github.com/user-attachments/assets/2585629f-e415-4fc3-a329-63f252981cb1" />

(AndrewG MemoryAllocator)

**This project implements an OS-backed memory allocator that creates a private heap using mmap. It is an independent learning project I carried out to better understand the mechanisms behind dynamic memory allocation**
---

The heap is managed with RAII. The constructor maps the requested heap size (which is rounded up to a page-aligned size), and the destructor unmaps it with munmap.

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




 

