#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "heap.hpp"

static volatile std::uint64_t sink = 0;

static inline void touch(void* p, std::size_t n) {
    if (!p || n == 0) return;

    auto* b = static_cast<std::uint8_t*>(p);
    b[0] ^= 0xA5;
    b[n / 2] ^= 0x5A;
    b[n - 1] ^= 0x3C;

    sink += b[0] + b[n / 2] + b[n - 1];
}

template <typename F>
long long time_us(F&& f) {
    auto t0 = std::chrono::steady_clock::now();
    f();
    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
}

struct Stats {
    long long min_us{};
    long long median_us{};
    long long max_us{};
    double avg_us{};
};

static Stats summarise(std::vector<long long> xs) {
    std::sort(xs.begin(), xs.end());

    Stats s;
    s.min_us = xs.front();
    s.max_us = xs.back();
    s.median_us = xs[xs.size() / 2];

    long long sum = std::accumulate(xs.begin(), xs.end(), 0LL);
    s.avg_us = static_cast<double>(sum) / static_cast<double>(xs.size());

    return s;
}

static void print_result(const char* name, const Stats& heap, const Stats& malloc_stats, long long ops) {
    std::printf("\n=== %s ===\n", name);

    std::printf("Heap   min/med/avg/max: %lld / %lld / %.1f / %lld us\n",
                heap.min_us, heap.median_us, heap.avg_us, heap.max_us);

    std::printf("malloc min/med/avg/max: %lld / %lld / %.1f / %lld us\n",
                malloc_stats.min_us, malloc_stats.median_us, malloc_stats.avg_us, malloc_stats.max_us);

    double ratio = static_cast<double>(malloc_stats.median_us) /
                   static_cast<double>(heap.median_us);

    double heap_ops_sec = static_cast<double>(ops) / (static_cast<double>(heap.median_us) / 1'000'000.0);
    double malloc_ops_sec = static_cast<double>(ops) / (static_cast<double>(malloc_stats.median_us) / 1'000'000.0);

    std::printf("Ratio malloc / heap: %.3fx\n", ratio);
    std::printf("Heap ops/sec:        %.2f M\n", heap_ops_sec / 1'000'000.0);
    std::printf("malloc ops/sec:      %.2f M\n", malloc_ops_sec / 1'000'000.0);
}

int main() {
    std::printf("- Preparing allocator benchmark -\n");

    constexpr AllocationPriority PRIORITY = AllocationPriority::FirstFit;

    constexpr int TRIALS = 9;
    constexpr int WARMUP_TRIALS = 2;

    constexpr int ITERS = 2000;
    constexpr int N = 2000;

    constexpr std::size_t MIN_SZ = 8;
    constexpr std::size_t MAX_SZ = 512;

    constexpr std::size_t HEAP_SIZE = 16 << 20;

    constexpr unsigned SEED = 67;

    std::mt19937 rng(SEED);
    std::uniform_int_distribution<std::size_t> size_dist(MIN_SZ, MAX_SZ);

    std::vector<std::size_t> sizes(N);
    for (int i = 0; i < N; ++i) {
        sizes[i] = size_dist(rng);
    }

    auto bench_heap_immediate = [&]() {
        Heap heap(HEAP_SIZE);

        return time_us([&] {
            for (int it = 0; it < ITERS; ++it) {
                for (int i = 0; i < N; ++i) {
                    std::size_t sz = sizes[i];
                    void* p = heap.alloc(sz, PRIORITY);
                    touch(p, sz);
                    heap.release(p);
                }
            }
        });
    };

    auto bench_malloc_immediate = [&]() {
        return time_us([&] {
            for (int it = 0; it < ITERS; ++it) {
                for (int i = 0; i < N; ++i) {
                    std::size_t sz = sizes[i];
                    void* p = std::malloc(sz);
                    touch(p, sz);
                    std::free(p);
                }
            }
        });
    };

    auto bench_heap_bulk = [&]() {
        Heap heap(HEAP_SIZE);

        return time_us([&] {
            for (int it = 0; it < ITERS; ++it) {
                std::vector<void*> ptrs(N, nullptr);

                for (int i = 0; i < N; ++i) {
                    ptrs[i] = heap.alloc(sizes[i], PRIORITY);
                    touch(ptrs[i], sizes[i]);
                }

                for (int i = 0; i < N; ++i) {
                    heap.release(ptrs[i]);
                }
            }
        });
    };

    auto bench_malloc_bulk = [&]() {
        return time_us([&] {
            for (int it = 0; it < ITERS; ++it) {
                std::vector<void*> ptrs(N, nullptr);

                for (int i = 0; i < N; ++i) {
                    ptrs[i] = std::malloc(sizes[i]);
                    touch(ptrs[i], sizes[i]);
                }

                for (int i = 0; i < N; ++i) {
                    std::free(ptrs[i]);
                }
            }
        });
    };

    auto bench_heap_fragmentation = [&]() {
        return time_us([&] {
            for (int it = 0; it < ITERS; ++it) {
                Heap heap(HEAP_SIZE);
                std::vector<void*> ptrs(N, nullptr);

                for (int i = 0; i < N; ++i) {
                    ptrs[i] = heap.alloc(sizes[i], PRIORITY);
                    touch(ptrs[i], sizes[i]);
                }

                for (int i = 0; i < N; i += 2) {
                    heap.release(ptrs[i]);
                    ptrs[i] = nullptr;
                }

                for (int i = 0; i < N / 2; ++i) {
                    std::size_t sz = sizes[(i * 7) % N];
                    void* p = heap.alloc(sz, PRIORITY);
                    touch(p, sz);
                    heap.release(p);
                }

                for (int i = 1; i < N; i += 2) {
                    heap.release(ptrs[i]);
                }
            }
        });
    };

    auto bench_malloc_fragmentation = [&]() {
        return time_us([&] {
            for (int it = 0; it < ITERS; ++it) {
                std::vector<void*> ptrs(N, nullptr);

                for (int i = 0; i < N; ++i) {
                    ptrs[i] = std::malloc(sizes[i]);
                    touch(ptrs[i], sizes[i]);
                }

                for (int i = 0; i < N; i += 2) {
                    std::free(ptrs[i]);
                    ptrs[i] = nullptr;
                }

                for (int i = 0; i < N / 2; ++i) {
                    std::size_t sz = sizes[(i * 7) % N];
                    void* p = std::malloc(sz);
                    touch(p, sz);
                    std::free(p);
                }

                for (int i = 1; i < N; i += 2) {
                    std::free(ptrs[i]);
                }
            }
        });
    };

    auto run_trials = [&](auto&& fn) {
        std::vector<long long> results;

        for (int i = 0; i < WARMUP_TRIALS; ++i) {
            fn();
        }

        for (int i = 0; i < TRIALS; ++i) {
            results.push_back(fn());
        }

        return summarise(results);
    };

    Stats heap_immediate = run_trials(bench_heap_immediate);
    Stats malloc_immediate = run_trials(bench_malloc_immediate);

    Stats heap_bulk = run_trials(bench_heap_bulk);
    Stats malloc_bulk = run_trials(bench_malloc_bulk);

    Stats heap_frag = run_trials(bench_heap_fragmentation);
    Stats malloc_frag = run_trials(bench_malloc_fragmentation);

    const long long immediate_ops = static_cast<long long>(ITERS) * N * 2;
    const long long bulk_ops = static_cast<long long>(ITERS) * N * 2;
    const long long frag_ops = static_cast<long long>(ITERS) * (N + N / 2 + N);

    std::printf("\n--- Benchmark results ---\n");
    std::printf("Trials: %d, warmups: %d, ITERS: %d, N: %d\n",
                TRIALS, WARMUP_TRIALS, ITERS, N);

    print_result("Immediate alloc/free", heap_immediate, malloc_immediate, immediate_ops);
    print_result("Bulk alloc then free", heap_bulk, malloc_bulk, bulk_ops);
    print_result("Fragmentation churn", heap_frag, malloc_frag, frag_ops);

    std::printf("\nChecksum sink: %llu\n", static_cast<unsigned long long>(sink));
    std::printf("- Program finished :) -\n");

    return 0;
}