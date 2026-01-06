#include "MemoryPool.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <vector>
#include <cstring>

using namespace std::chrono;

#define GREEN "\033[32m"
#define CYAN "\033[36m"
#define YELLOW "\033[33m"
#define BOLD "\033[1m"
#define RESET "\033[0m"

// Volatile to prevent optimization
volatile int sink = 0;


inline void use_pointer(void* ptr) {

    if (ptr) {
        *(reinterpret_cast<volatile char*>(ptr)) = 1;
    }
}

void printResult(const std::string& name, double mallocMs, double poolMs, size_t ops) {
    double speedup = mallocMs / poolMs;
    std::string color = (speedup >= 1.0) ? GREEN : YELLOW;
    
    std::cout << std::left << std::setw(40) << name
              << std::right << std::setw(12) << std::fixed << std::setprecision(2) << mallocMs
              << std::setw(12) << poolMs
              << color << std::setw(12) << speedup << "x" << RESET << "\n";
}

// Benchmark: Ultra-tight single allocation loop
void benchmarkUltraTight() {
    const size_t ITERATIONS = 10000000;  // 10 million
    const size_t BLOCK_SIZE = 32;
    
    // malloc benchmark
    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        void* p = std::malloc(BLOCK_SIZE);
        use_pointer(p);  // Prevent optimization
        std::free(p);
    }
    auto end = high_resolution_clock::now();
    double mallocTime = duration_cast<microseconds>(end - start).count() / 1000.0;
    
    // Pool benchmark
    MemoryPool pool(BLOCK_SIZE, 1);  // Single block, maximum reuse!
    start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        void* p = pool.allocate();
        use_pointer(p);  // Prevent optimization
        pool.deallocate(p);
    }
    end = high_resolution_clock::now();
    double poolTime = duration_cast<microseconds>(end - start).count() / 1000.0;
    
    printResult("Ultra-Tight Loop (32B, 10M ops)", mallocTime, poolTime, ITERATIONS * 2);
}

// Benchmark: Paired allocations
void benchmarkPaired() {
    const size_t ITERATIONS = 5000000;
    const size_t BLOCK_SIZE = 16;
    
    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        void* p1 = std::malloc(BLOCK_SIZE);
        void* p2 = std::malloc(BLOCK_SIZE);
        use_pointer(p1);
        use_pointer(p2);
        std::free(p2);
        std::free(p1);
    }
    auto end = high_resolution_clock::now();
    double mallocTime = duration_cast<microseconds>(end - start).count() / 1000.0;
    
    MemoryPool pool(BLOCK_SIZE, 2);
    start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        void* p1 = pool.allocate();
        void* p2 = pool.allocate();
        use_pointer(p1);
        use_pointer(p2);
        pool.deallocate(p2);
        pool.deallocate(p1);
    }
    end = high_resolution_clock::now();
    double poolTime = duration_cast<microseconds>(end - start).count() / 1000.0;
    
    printResult("Paired Allocations (16B, 5M ops)", mallocTime, poolTime, ITERATIONS * 4);
}

// Benchmark: Tiny objects
void benchmarkTiny() {
    const size_t ITERATIONS = 10000000;
    const size_t BLOCK_SIZE = 8;
    
    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        void* p = std::malloc(BLOCK_SIZE);
        use_pointer(p);
        std::free(p);
    }
    auto end = high_resolution_clock::now();
    double mallocTime = duration_cast<microseconds>(end - start).count() / 1000.0;
    
    MemoryPool pool(BLOCK_SIZE, 1);
    start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        void* p = pool.allocate();
        use_pointer(p);
        pool.deallocate(p);
    }
    end = high_resolution_clock::now();
    double poolTime = duration_cast<microseconds>(end - start).count() / 1000.0;
    
    printResult("Tiny Objects (8B, 10M ops)", mallocTime, poolTime, ITERATIONS * 2);
}

// Benchmark: Stack simulation
void benchmarkStack() {
    const size_t ITERATIONS = 1000000;
    const size_t DEPTH = 10;
    const size_t BLOCK_SIZE = 64;
    
    void* stack[DEPTH];
    
    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        for (size_t j = 0; j < DEPTH; ++j) {
            stack[j] = std::malloc(BLOCK_SIZE);
            use_pointer(stack[j]);
        }
        for (int j = DEPTH - 1; j >= 0; --j) {
            std::free(stack[j]);
        }
    }
    auto end = high_resolution_clock::now();
    double mallocTime = duration_cast<microseconds>(end - start).count() / 1000.0;
    
    MemoryPool pool(BLOCK_SIZE, DEPTH);
    start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        for (size_t j = 0; j < DEPTH; ++j) {
            stack[j] = pool.allocate();
            use_pointer(stack[j]);
        }
        for (int j = DEPTH - 1; j >= 0; --j) {
            pool.deallocate(stack[j]);
        }
    }
    end = high_resolution_clock::now();
    double poolTime = duration_cast<microseconds>(end - start).count() / 1000.0;
    
    printResult("Stack Pattern (64B, depth=10)", mallocTime, poolTime, ITERATIONS * DEPTH * 2);
}

// Benchmark: Rapid fire
void benchmarkRapidFire() {
    const size_t ITERATIONS = 2000000;
    const size_t BLOCK_SIZE = 24;
    
    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        void* p1 = std::malloc(BLOCK_SIZE);
        void* p2 = std::malloc(BLOCK_SIZE);
        void* p3 = std::malloc(BLOCK_SIZE);
        use_pointer(p1);
        use_pointer(p2);
        use_pointer(p3);
        std::free(p1);
        std::free(p2);
        std::free(p3);
    }
    auto end = high_resolution_clock::now();
    double mallocTime = duration_cast<microseconds>(end - start).count() / 1000.0;
    
    MemoryPool pool(BLOCK_SIZE, 3);
    start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        void* p1 = pool.allocate();
        void* p2 = pool.allocate();
        void* p3 = pool.allocate();
        use_pointer(p1);
        use_pointer(p2);
        use_pointer(p3);
        pool.deallocate(p1);
        pool.deallocate(p2);
        pool.deallocate(p3);
    }
    end = high_resolution_clock::now();
    double poolTime = duration_cast<microseconds>(end - start).count() / 1000.0;
    
    printResult("Rapid Fire (24B, 3 per iter)", mallocTime, poolTime, ITERATIONS * 6);
}

// Benchmark: Single byte allocations
void benchmarkSingleByte() {
    const size_t ITERATIONS = 5000000;
    const size_t BLOCK_SIZE = 1;
    
    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        void* p = std::malloc(BLOCK_SIZE);
        use_pointer(p);
        std::free(p);
    }
    auto end = high_resolution_clock::now();
    double mallocTime = duration_cast<microseconds>(end - start).count() / 1000.0;
    
    MemoryPool pool(BLOCK_SIZE, 1);
    start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        void* p = pool.allocate();
        use_pointer(p);
        pool.deallocate(p);
    }
    end = high_resolution_clock::now();
    double poolTime = duration_cast<microseconds>(end - start).count() / 1000.0;
    
    printResult("Single Byte (1B, 5M ops)", mallocTime, poolTime, ITERATIONS * 2);
}

// Benchmark: Write actual data (most realistic)
void benchmarkWithWrites() {
    const size_t ITERATIONS = 5000000;
    const size_t BLOCK_SIZE = 64;
    
    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        int* p = static_cast<int*>(std::malloc(BLOCK_SIZE));
        if (p) {
            *p = i;  // Write data
            sink += *p;  // Use data
        }
        std::free(p);
    }
    auto end = high_resolution_clock::now();
    double mallocTime = duration_cast<microseconds>(end - start).count() / 1000.0;
    
    MemoryPool pool(BLOCK_SIZE, 1);
    start = high_resolution_clock::now();
    for (size_t i = 0; i < ITERATIONS; ++i) {
        int* p = static_cast<int*>(pool.allocate());
        if (p) {
            *p = i;  // Write data
            sink += *p;  // Use data
        }
        pool.deallocate(p);
    }
    end = high_resolution_clock::now();
    double poolTime = duration_cast<microseconds>(end - start).count() / 1000.0;
    
    printResult("With Data Writes (64B, 5M ops)", mallocTime, poolTime, ITERATIONS * 2);
}

int main() {
    std::cout << BOLD << CYAN;
    std::cout << "╔════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║       Memory Pool - EXTREME Performance Benchmark (Optimized)     ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════════╝\n";
    std::cout << RESET << "\n";
    
    std::cout << YELLOW << "Optimizations:\n";
    std::cout << "  ✓ Removed std::unordered_set tracking\n";
    std::cout << "  ✓ Minimized safety checks\n";
    std::cout << "  ✓ Pure pointer arithmetic\n";
    std::cout << "  ✓ Tiny pool sizes for maximum cache hits\n";
    std::cout << "  ✓ Prevents compiler optimization with memory writes\n\n" << RESET;
    
    std::cout << BOLD << std::string(76, '=') << RESET << "\n";
    std::cout << std::left << std::setw(40) << "Benchmark"
              << std::right << std::setw(12) << "malloc(ms)"
              << std::setw(12) << "Pool(ms)"
              << std::setw(12) << "Speedup\n";
    std::cout << std::string(76, '-') << "\n";
    
    benchmarkUltraTight();
    benchmarkTiny();
    benchmarkSingleByte();
    benchmarkPaired();
    benchmarkRapidFire();
    benchmarkStack();
    benchmarkWithWrites();
    
    std::cout << std::string(76, '=') << "\n\n";
    
    std::cout << GREEN << BOLD << "Expected Results:\n" << RESET;
    std::cout << "  • Ultra-tight loops: 2-5x speedup\n";
    std::cout << "  • Tiny allocations: 3-8x speedup\n";
    std::cout << "  • Small pools (1-10 blocks): Best performance\n\n";
    
    std::cout << YELLOW << "Note: Modern malloc (glibc 2.x+) is highly optimized.\n";
    std::cout << "Memory pools shine in:\n";
    std::cout << "  - Embedded systems without optimized malloc\n";
    std::cout << "  - Real-time systems needing deterministic timing\n";
    std::cout << "  - Applications requiring zero fragmentation\n" << RESET;
    
    return 0;
}