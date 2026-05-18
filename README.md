## A simple memory allocator

This learning project implements a custom memory allocator that manages its own heap using virtual memory obtained through the mmap system call.

The heap is managed with RAII. The constructor maps the requested heap size (rounded up to a page-aligned size). The destructor releases the mapping using munmap.

Memory is organised into blocks with the following layout:

- **Header** – stores metadata about the block (size, allocation status, free-list pointers).
- **Footer** – mirrors the block size so the previous block can be located in **O(1)** time during coalescing.

---
## Allocation Strategies

### First-Fit
- Selects the first block large enough, terminates search early, with the benefit of lower overhead under fragmentation

### Best-Fit
- Selects the smallest suitable block, requires scanning entire free list, with better theoretical space utilisation

---

## Benchmark Results vs `malloc` / `free`

On the Macbook M3 Pro Chip 


Benchmarks were run using randomized allocation sizes between **8–512 bytes**, with multiple trials and median reporting.

| Configuration | Iterations | Operations / Iteration |
|---------------|-----------|------------------------|
| Benchmark Setup | 2,000 | 2,000 |

---

## First-Fit Allocation Policy

| Test Scenario | Result vs `malloc/free` |
|---------------|------------------------|
| Immediate Allocation / Deallocation | **11.42× faster** |
| Bulk Allocation / Deallocation | **5.00× faster** |
| Fragmented Allocation / Deallocation | **1.99× faster** |

---

## Best-Fit Allocation Policy

| Test Scenario | Result vs `malloc/free` |
|---------------|------------------------|
| Immediate Allocation / Deallocation | **8.89× faster** |
| Bulk Allocation / Deallocation | **5.71× faster** |
| Fragmented Allocation / Deallocation | **15.4× slower** |

---
