#include "MemoryPool.h"
#include <cstdlib>
#include <iostream>
#include <cstring>

// OPTIMIZED VERSION - Removes tracking overhead for maximum performance
// Trade-off: No double-free detection, minimal safety checks

MemoryPool::MemoryPool(size_t blockSize, size_t numBlocks, bool threadSafe)
    : memoryStart(nullptr)              
    , freeList(nullptr)                 
    , blockSize(alignSize(blockSize))   
    , totalBlocks(numBlocks)            
    , freeBlockCount(numBlocks)         
    , threadSafe(threadSafe) {          
    
    if (blockSize < sizeof(Block*)) {
        this->blockSize = alignSize(sizeof(Block*));
    }
    
    if (numBlocks == 0) {
        throw std::invalid_argument("Number of blocks must be greater than 0");
    }

    // Allocate one contiguous chunk of memory
    memoryStart = std::malloc(this->blockSize * numBlocks);
    if (!memoryStart) {
        throw std::bad_alloc();
    }
    
    // Initialize free list by linking all blocks
    freeList = static_cast<Block*>(memoryStart);
    Block* current = freeList;
    
    for (size_t i = 0; i < numBlocks - 1; ++i) {
        // Calculate next block address
        void* nextAddr = static_cast<char*>(static_cast<void*>(current)) + this->blockSize;
        current->next = static_cast<Block*>(nextAddr);
        current = current->next;
    }
    current->next = nullptr;  // Last block points to null
}

MemoryPool::~MemoryPool() {
    // Simple check for leaks
    if (freeBlockCount != totalBlocks) {
        std::cerr << "WARNING: Memory leak detected! "
                  << (totalBlocks - freeBlockCount) << " blocks not freed.\n";
    }
    
    // Free the entire memory pool
    if (memoryStart) {
        std::free(memoryStart);
    }
}

void* MemoryPool::allocate() {
    if (threadSafe) {
        std::lock_guard<std::mutex> lock(poolMutex);
        return allocateInternal();
    }
    return allocateInternal();
}

void* MemoryPool::allocateInternal() {
    // Check if pool is exhausted
    if (!freeList) {
        return nullptr;
    }
    
    // Pop from free list - FAST PATH
    void* block = freeList;
    freeList = freeList->next;
    --freeBlockCount;
    
    return block;
}

void MemoryPool::deallocate(void* ptr) {
    if (threadSafe) {
        std::lock_guard<std::mutex> lock(poolMutex);
        deallocateInternal(ptr);
        return;
    }
    deallocateInternal(ptr);
}

void MemoryPool::deallocateInternal(void* ptr) {
    if (!ptr) {
        return;
    }
    
    // OPTIONAL: Basic bounds check (comment out for maximum speed)
    #ifdef MEMPOOL_SAFE_MODE
    char* ptrAddr = static_cast<char*>(ptr);
    char* startAddr = static_cast<char*>(memoryStart);
    char* endAddr = startAddr + (blockSize * totalBlocks);
    
    if (ptrAddr < startAddr || ptrAddr >= endAddr) {
        throw std::invalid_argument("Pointer not from this pool");
    }
    #endif
    
    // Push to free list - FAST PATH
    Block* block = static_cast<Block*>(ptr);
    block->next = freeList;
    freeList = block;
    ++freeBlockCount;
}

void MemoryPool::reset() {
    if (threadSafe) {
        std::lock_guard<std::mutex> lock(poolMutex);
   	 freeBlockCount = totalBlocks;
    
	    // Rebuild free list
	    freeList = static_cast<Block*>(memoryStart);
	    Block* current = freeList;
	    
	    for (size_t i = 0; i < totalBlocks - 1; ++i) {
		void* nextAddr = static_cast<char*>(static_cast<void*>(current)) + blockSize;
		current->next = static_cast<Block*>(nextAddr);
		current = current->next;
	    }
	    current->next = nullptr;

    }
    
    freeBlockCount = totalBlocks;
    
    // Rebuild free list
    freeList = static_cast<Block*>(memoryStart);
    Block* current = freeList;
    
    for (size_t i = 0; i < totalBlocks - 1; ++i) {
        void* nextAddr = static_cast<char*>(static_cast<void*>(current)) + blockSize;
        current->next = static_cast<Block*>(nextAddr);
        current = current->next;
    }
    current->next = nullptr;
}

size_t MemoryPool::alignSize(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}
