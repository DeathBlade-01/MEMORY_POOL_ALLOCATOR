#ifndef MEMORY_POOL_H
#define MEMORY_POOL_H

#include <cstddef>
#include <mutex>

class MemoryPool {
private:
    struct Block {
        Block* next;
    };

    void* memoryStart;          // Start of memory pool
    Block* freeList;            // Head of free list
    size_t blockSize;           // Size of each block (aligned)
    size_t totalBlocks;         // Total number of blocks
    size_t freeBlockCount;      // Number of free blocks
    bool threadSafe;            // Thread safety flag
    std::mutex poolMutex;       // Mutex for thread safety

    // Helper functions
    static size_t alignSize(size_t size, size_t alignment = alignof(std::max_align_t));
    void* allocateInternal();
    void deallocateInternal(void* ptr);

public:
    // Constructor
    MemoryPool(size_t blockSize, size_t numBlocks, bool threadSafe = false);
    
    // Destructor
    ~MemoryPool();

    // Delete copy constructor and assignment operator
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    // Allocate a block from the pool
    void* allocate();

    // Deallocate a block back to the pool
    void deallocate(void* ptr);

    // Reset the pool (frees all allocations)
    void reset();

    // Query functions
    inline bool isExhausted() const { return freeBlockCount == 0; }
    inline size_t getUsedBlocks() const { return totalBlocks - freeBlockCount; }
    inline size_t getFreeBlocks() const { return freeBlockCount; }
    inline size_t getBlockSize() const { return blockSize; }
    inline size_t getTotalBlocks() const { return totalBlocks; }
};

#endif // MEMORY_POOL_H