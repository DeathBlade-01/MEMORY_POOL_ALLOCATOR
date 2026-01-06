# Memory Pool Allocator

A fast, simple memory allocator for fixed-size blocks that's **2-3x faster** than standard `malloc`/`free`.

## What's the Problem with malloc?

Every time you call `malloc()`:
1. It searches through data structures to find free memory
2. It makes system calls to the operating system
3. It maintains complex metadata about each allocation
4. It tries to handle any size you throw at it

This is **slow** when you're allocating and freeing the same size over and over.

## How Does Memory Pool Work?

Think of it like a parking lot with pre-marked spaces:

```
Traditional malloc:        Memory Pool:
  Find space → Park          Space already ready → Park
  (slow)                     (fast)

┌─────────────┐            ┌───┬───┬───┬───┬───┐
│ Search for  │            │ 1 │ 2 │ 3 │ 4 │ 5 │  Pre-allocated
│ free space  │            └───┴───┴───┴───┴───┘  Fixed spots
│   ...       │                   ↓
└─────────────┘            Just grab the next one!
```

### Key Differences

| malloc | Memory Pool |
|--------|-------------|
| Any size allocation | Fixed size only |
| Searches for space | Pre-allocated spots |
| System calls | Pure pointer swap |
| Complex metadata | Simple linked list |
| ~100-200 CPU cycles | ~10-20 CPU cycles |

## Performance Results

From our benchmarks on a typical Linux server:

```
Benchmark                                 malloc(ms)    Pool(ms)    Speedup
----------------------------------------------------------------------------
Ultra-Tight Loop (32B, 10M ops)               209.13       79.10      2.64x
Tiny Objects (8B, 10M ops)                    197.40       73.44      2.69x
Single Byte (1B, 5M ops)                      111.96       35.66      3.14x
Paired Allocations (16B, 5M ops)              215.75       73.60      2.93x
```

**Average: 2.7x faster than malloc**

## How It Works (Simple Explanation)

1. **At startup**: Allocate one big chunk of memory, divide it into equal blocks
2. **To allocate**: Pop a block from the free list (just one pointer operation)
3. **To free**: Push the block back to the free list (just one pointer operation)

```cpp
// The entire allocation is basically:
void* allocate() {
    Block* result = freeList;    // Get first free block
    freeList = freeList->next;   // Move to next
    return result;                // Done! (~3 instructions)
}
```

Compare this to malloc which might do hundreds of instructions with complex logic!

## Quick Start

```cpp
#include "MemoryPool.h"

// Create a pool for 64-byte objects, 1000 total
MemoryPool pool(64, 1000);

// Allocate (fast!)
void* ptr = pool.allocate();

// Use it...
MyObject* obj = static_cast<MyObject*>(ptr);

// Free it (fast!)
pool.deallocate(ptr);
```

## When Should You Use This?

✅ **Good for:**
- Game objects (bullets, particles, enemies)
- Network packets
- Small database records
- Event queues
- Any time you allocate/free the **same size** repeatedly

❌ **Not good for:**
- Variable-sized allocations
- One-time allocations
- When you don't know the size in advance

## Building

```bash
# Compile with optimizations
g++ -std=c++11 -O3 MemoryPool_MK2.cpp your_code.cpp -o your_program

# Run tests
g++ -std=c++11 -O3 MemoryPool_MK2.cpp tests.cpp -o tests
./tests

# Run benchmarks
g++ -std=c++11 -O3 MemoryPool_MK2.cpp BenchMark.cpp -o benchmark
./benchmark
```

## Features

- **Thread-safe option**: Add `true` parameter for multi-threaded use
- **Zero fragmentation**: Pre-allocated memory means no fragmentation
- **Predictable timing**: Every allocation takes the same time (good for real-time systems)
- **Memory leak detection**: Warns you if you forget to free blocks


## Files

- `MemoryPool.h` - Header file
- `MemoryPool_MK2.cpp` - Optimized implementation (no tracking overhead)
- `tests.cpp` - Unit tests
- `BenchMark.cpp` - Performance benchmarks


**Bottom line**: If you're allocating the same size repeatedly, this is ~3x faster than malloc. Simple as that.
