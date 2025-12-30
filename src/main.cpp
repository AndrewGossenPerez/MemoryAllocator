#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <random>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include "include/heap.hpp"

static volatile std::uint64_t sink = 0;

static inline void touch(void* p, std::size_t n) {
    // Touch a few bytes so the compiler wont optimise away the allocation 
    if (!p || n == 0) return;
    auto* b = static_cast<std::uint8_t*>(p);
    b[0] ^= 0xA5;
    b[n / 2] ^= 0x5A;
    b[n - 1] ^= 0x3C;
    sink += b[0] + b[n / 2] + b[n - 1];
}

template <typename F>
static long long time_us(F&& f) {
    auto t0 = std::chrono::steady_clock::now();
    f();
    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
}

int main() { // Benchmarking 

    std::printf("- Preparing to benchmark -\n");

    // --- Benchmark setup ---

    constexpr AllocationPriority prioryTesting=AllocationPriority::BestFit;
    constexpr int ITERS = 10000; // Multiple iterations done to smooth noise 
    constexpr int N = 10000; // Amount of operations per iteration
    constexpr std::size_t MIN_SZ = 8; // Smallest payload size 
    constexpr std::size_t MAX_SZ = 256; // Largest payload size 
    constexpr unsigned SEED = 67;

    std::mt19937 rng(SEED);
    std::uniform_int_distribution<std::size_t> dist(MIN_SZ, MAX_SZ);

    std::vector<std::size_t> sizes;
    sizes.reserve(N);
    for (int i = 0; i < N; ++i) sizes.push_back(dist(rng));

    // Immediate alloc/free for a throughput test 
    long long heap_us = time_us([&] {
        for (int it = 0; it < ITERS; ++it) {
            Heap heap(4 << 20); 
            for (int i = 0; i < N; ++i) {
                std::size_t sz = sizes[i];
                void* p = heap.alloc(sz,prioryTesting);
                touch(p, sz);
                heap.release(p);
            }
        }
    });

    long long malloc_us = time_us([&] {
        for (int it = 0; it < ITERS; ++it) {
            for (int i = 0; i < N; ++i) {
                std::size_t sz = sizes[i];
                void* p = std::malloc(sz);
                touch(p, sz);
                std::free(p);
            }
        }
    });

    // Framgmentation pattern
    long long heap_frag_us = time_us([&] {
        for (int it = 0; it < ITERS; ++it) {
            Heap heap(8 << 20);
            std::vector<void*> ptrs(N, nullptr);

            // Bulk alloc
            for (int i = 0; i < N; ++i) {
                ptrs[i] = heap.alloc(sizes[i], prioryTesting);
                touch(ptrs[i], sizes[i]);
            }

            // Free every other to create holes
            for (int i = 0; i < N; i += 2) heap.release(ptrs[i]);

            // Allocate into holes then free immediately
            for (int i = 0; i < N / 2; ++i) {
                std::size_t sz = sizes[i];
                void* p = heap.alloc(sz, prioryTesting);
                touch(p, sz);
                heap.release(p);
            }

            // Free remainder
            for (int i = 1; i < N; i += 2) heap.release(ptrs[i]);

        }
    });

    long long malloc_frag_us = time_us([&] {
        for (int it = 0; it < ITERS; ++it) {
            std::vector<void*> ptrs(N, nullptr);

            for (int i = 0; i < N; ++i) {
                ptrs[i] = std::malloc(sizes[i]);
                touch(ptrs[i], sizes[i]);
            }

            for (int i = 0; i < N; i += 2) std::free(ptrs[i]);

            for (int i = 0; i < N / 2; ++i) {
                std::size_t sz = sizes[i];
                void* p = std::malloc(sz);
                touch(p, sz);
                std::free(p);
            }

            for (int i = 1; i < N; i += 2) std::free(ptrs[i]);
        }
    });

    std::printf("--- Benchmark results (in microseconds) ---- \n");
    std::printf(" -Heap immediate free : %lld us\n", heap_us);
    std::printf(" -malloc & free immediate free : %lld us\n", malloc_us);
    std::printf(" -Heap fragmentation : %lld us\n", heap_frag_us);
    std::printf(" -malloc & free fragmentation : %lld us\n", malloc_frag_us);

    std::printf("\n-- Ratios (Heap / malloc) --\n");
    std::printf(" -Immediate: %.3fx faster than malloc & free\n", (double)malloc_us/(double)heap_us);
    std::printf(" -Fragment: %.3fx faster than malloc & free\n", (double)malloc_frag_us/(double)heap_frag_us);

    std::printf("- Program finished :) -\n");

    return 0;

}
