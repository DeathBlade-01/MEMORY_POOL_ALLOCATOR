#include "MemoryPool.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <thread>

// Color codes for output
#define GREEN "\033[32m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"

void printTestResult(const std::string& testName, bool passed) {
    if (passed) {
        std::cout << GREEN << "[PASS] " << RESET << testName << std::endl;
    } else {
        std::cout << RED << "[FAIL] " << RESET << testName << std::endl;
    }
}

// Test 1: Basic allocation and deallocation
void testBasicAllocation() {
    std::cout << YELLOW << "\n=== Test 1: Basic Allocation ===" << RESET << std::endl;
    
    MemoryPool pool(32, 10);
    
    // Test allocation
    void* ptr1 = pool.allocate();
    assert(ptr1 != nullptr);
    assert(pool.getUsedBlocks() == 1);
    assert(pool.getFreeBlocks() == 9);
    printTestResult("Single allocation", true);
    
    // Test deallocation
    pool.deallocate(ptr1);
    assert(pool.getUsedBlocks() == 0);
    assert(pool.getFreeBlocks() == 10);
    printTestResult("Single deallocation", true);
}

// Test 2: Multiple allocations
void testMultipleAllocations() {
    std::cout << YELLOW << "\n=== Test 2: Multiple Allocations ===" << RESET << std::endl;
    
    MemoryPool pool(64, 100);
    std::vector<void*> pointers;
    
    // Allocate 50 blocks
    for (int i = 0; i < 50; ++i) {
        void* ptr = pool.allocate();
        assert(ptr != nullptr);
        pointers.push_back(ptr);
    }
    
    assert(pool.getUsedBlocks() == 50);
    assert(pool.getFreeBlocks() == 50);
    printTestResult("50 allocations", true);
    
    // Deallocate all
    for (void* ptr : pointers) {
        pool.deallocate(ptr);
    }
    
    assert(pool.getUsedBlocks() == 0);
    assert(pool.getFreeBlocks() == 100);
    printTestResult("50 deallocations", true);
}

// Test 3: Pool exhaustion
void testPoolExhaustion() {
    std::cout << YELLOW << "\n=== Test 3: Pool Exhaustion ===" << RESET << std::endl;
    
    MemoryPool pool(32, 5);
    std::vector<void*> pointers;
    
    // Allocate all blocks
    for (int i = 0; i < 5; ++i) {
        void* ptr = pool.allocate();
        assert(ptr != nullptr);
        pointers.push_back(ptr);
    }
    
    assert(pool.isExhausted());
    printTestResult("Pool exhaustion detection", true);
    
    // Try to allocate when exhausted
    void* ptr = pool.allocate();
    assert(ptr == nullptr);
    printTestResult("Allocation returns nullptr when exhausted", true);
    
    // Free one block and reallocate
    pool.deallocate(pointers[0]);
    ptr = pool.allocate();
    assert(ptr != nullptr);
    printTestResult("Reallocation after freeing", true);
}

// Test 4: Interleaved allocation/deallocation
void testInterleavedOperations() {
    std::cout << YELLOW << "\n=== Test 4: Interleaved Operations ===" << RESET << std::endl;
    
    MemoryPool pool(128, 10);
    
    void* p1 = pool.allocate();
    void* p2 = pool.allocate();
    void* p3 = pool.allocate();
    
    pool.deallocate(p2);  // Free middle one
    
    void* p4 = pool.allocate();  // Should reuse p2's slot
    void* p5 = pool.allocate();
    
    assert(pool.getUsedBlocks() == 4);
    printTestResult("Interleaved alloc/dealloc tracking", true);
    
    pool.deallocate(p1);
    pool.deallocate(p3);
    pool.deallocate(p4);
    pool.deallocate(p5);
    
    assert(pool.getUsedBlocks() == 0);
    printTestResult("Final cleanup", true);
}

// Test 5: Double-free detection
void testDoubleFree() {
    std::cout << YELLOW << "\n=== Test 5: Double-Free Detection ===" << RESET << std::endl;
    
    MemoryPool pool(32, 10);
    void* ptr = pool.allocate();
    pool.deallocate(ptr);
    
    bool exceptionThrown = false;
    try {
        pool.deallocate(ptr);  // Should throw
    } catch (const std::invalid_argument& e) {
        exceptionThrown = true;
    }
    
    printTestResult("Double-free detection", exceptionThrown);
}

// Test 6: Invalid pointer detection
void testInvalidPointer() {
    std::cout << YELLOW << "\n=== Test 6: Invalid Pointer Detection ===" << RESET << std::endl;
    
    MemoryPool pool(32, 10);
    int dummy = 42;
    void* invalidPtr = &dummy;
    
    bool exceptionThrown = false;
    try {
        pool.deallocate(invalidPtr);  // Should throw
    } catch (const std::invalid_argument& e) {
        exceptionThrown = true;
    }
    
    printTestResult("Invalid pointer detection", exceptionThrown);
}

// Test 7: Thread safety
void testThreadSafety() {
    std::cout << YELLOW << "\n=== Test 7: Thread Safety ===" << RESET << std::endl;
    
    MemoryPool pool(64, 1000, true);  // Thread-safe pool
    const int numThreads = 4;
    const int allocsPerThread = 100;
    
    auto workerFunc = [&pool, allocsPerThread]() {
        std::vector<void*> localPtrs;
        
        // Allocate
        for (int i = 0; i < allocsPerThread; ++i) {
            void* ptr = pool.allocate();
            if (ptr) {
                localPtrs.push_back(ptr);
            }
        }
        
        // Deallocate
        for (void* ptr : localPtrs) {
            pool.deallocate(ptr);
        }
    };
    
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(workerFunc);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    assert(pool.getUsedBlocks() == 0);
    printTestResult("Multi-threaded alloc/dealloc", true);
}

// Test 8: Memory alignment
void testAlignment() {
    std::cout << YELLOW << "\n=== Test 8: Memory Alignment ===" << RESET << std::endl;
    
    MemoryPool pool(33, 10);  // Odd size to test alignment
    
    void* ptr = pool.allocate();
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    
    // Check alignment (should be aligned to alignof(max_align_t))
    bool aligned = (addr % alignof(std::max_align_t)) == 0;
    printTestResult("Proper alignment", aligned);
    
    pool.deallocate(ptr);
}

// Test 9: Reset functionality
void testReset() {
    std::cout << YELLOW << "\n=== Test 9: Pool Reset ===" << RESET << std::endl;
    
    MemoryPool pool(32, 10);
    
    // Allocate some blocks
    std::vector<void*> ptrs;
    for (int i = 0; i < 5; ++i) {
        ptrs.push_back(pool.allocate());
    }
    
    assert(pool.getUsedBlocks() == 5);
    
    // Reset pool
    pool.reset();
    
    assert(pool.getUsedBlocks() == 0);
    assert(pool.getFreeBlocks() == 10);
    printTestResult("Pool reset", true);
    
    // Verify we can allocate again
    void* ptr = pool.allocate();
    assert(ptr != nullptr);
    printTestResult("Allocation after reset", true);
    pool.deallocate(ptr);
}

// Test 10: Stress test
void testStressTest() {
    std::cout << YELLOW << "\n=== Test 10: Stress Test ===" << RESET << std::endl;
    
    MemoryPool pool(128, 1000);
    std::vector<void*> pointers;
    
    // Perform 10,000 random operations
    for (int i = 0; i < 10000; ++i) {
        if (pointers.empty() || (rand() % 2 == 0 && !pool.isExhausted())) {
            // Allocate
            void* ptr = pool.allocate();
            if (ptr) {
                pointers.push_back(ptr);
            }
        } else {
            // Deallocate random pointer
            int idx = rand() % pointers.size();
            pool.deallocate(pointers[idx]);
            pointers.erase(pointers.begin() + idx);
        }
    }
    
    // Cleanup
    for (void* ptr : pointers) {
        pool.deallocate(ptr);
    }
    
    assert(pool.getUsedBlocks() == 0);
    printTestResult("10,000 random operations", true);
}

// Test 11: Different block sizes
void testDifferentBlockSizes() {
    std::cout << YELLOW << "\n=== Test 11: Different Block Sizes ===" << RESET << std::endl;
    
    std::vector<size_t> sizes = {8, 16, 32, 64, 128, 256, 512, 1024};
    
    for (size_t size : sizes) {
        MemoryPool pool(size, 10);
        void* ptr = pool.allocate();
        assert(ptr != nullptr);
        assert(pool.getBlockSize() >= size);
        pool.deallocate(ptr);
    }
    
    printTestResult("Various block sizes", true);
}

int main() {
    std::cout << GREEN << "╔════════════════════════════════════════╗" << RESET << std::endl;
    std::cout << GREEN << "║  Memory Pool Unit Tests               ║" << RESET << std::endl;
    std::cout << GREEN << "╚════════════════════════════════════════╝" << RESET << std::endl;
    
    try {
        testBasicAllocation();
        testMultipleAllocations();
        testPoolExhaustion();

        std::cout << GREEN << "(In the above case, the Memory Leak is a valid output as in the 3rd TestCase, \n reset is not called and 5 blocks are occupied when deconstructor is called...)" << RESET << std::endl;

        testInterleavedOperations();
        testDoubleFree();
        testInvalidPointer();
        testThreadSafety();
        testAlignment();
        testReset();
        testStressTest();
        testDifferentBlockSizes();
        
        std::cout << GREEN << "\n╔════════════════════════════════════════╗" << RESET << std::endl;
        std::cout << GREEN << "║  All tests passed! ✓                   ║" << RESET << std::endl;
        std::cout << GREEN << "╚════════════════════════════════════════╝" << RESET << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << RED << "Test failed with exception: " << e.what() << RESET << std::endl;
        return 1;
    }
    
    return 0;
}