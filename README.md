# Custom Memory Allocator in C

A malloc/free replacement built from scratch in C, using `mmap` for heap 
management and a free-list for allocation tracking. 
Replaces glibc's allocator via `LD_PRELOAD`.

## Architecture

### block_meta (Block Header)
Every allocation carries a 24-byte header stored immediately before 
the returned pointer:

struct block_meta {
    size_t size;        // usable bytes in this block
    struct block_meta *next;  // next block in free list
    int free;           // 1 = available, 0 = in use
};

### Free List
All allocated blocks are tracked in a singly linked list. 
On every malloc call, the list is walked first (first-fit search) 
before asking the OS for new memory.

### Splitting
When a free block is larger than requested, it is split into two:
- Block A: exactly the requested size (returned to caller)
- Block B: remainder, marked free, inserted into list

Example: 32-byte free block, 8-byte request →
Block A: 8 bytes (used) + 24 byte header
Block B: 0 bytes (free) + 24 byte header

### Coalescing
On free(), if the next block in the list is also free, 
the two blocks are merged into one:
merged size = block->size + sizeof(block_meta) + next->size

### Thread Safety
A pthread mutex (alloc_lock) protects all list operations, 
making the allocator safe for multi-threaded programs.

## Build

# Build shared library for LD_PRELOAD
gcc -Wall -Wextra -fPIC -shared -o myalloc.so dumb_alloc.c -lpthread

# Build test binary
gcc -Wall -Wextra -DTESTING -o test dumb_alloc.c -lpthread

## Usage with LD_PRELOAD

LD_PRELOAD=./myalloc.so ./your_program

This intercepts all malloc/free/calloc/realloc calls and routes 
them through this allocator instead of glibc.

## Benchmark

Stress test: 5 rounds of 10,000 random allocations (8-1024 bytes each)

| Allocator      | Time     |
|----------------|----------|
| glibc malloc   | 0.007s   |
| this allocator | 10.104s  |

## Why It's Slower Than glibc

This allocator calls mmap() for every allocation that misses the free list.
mmap() is a syscall — it crosses the kernel boundary every time.

glibc calls mmap() rarely (in large 128KB+ chunks) and carves those 
chunks internally without syscall overhead per allocation.

50,000 allocations × mmap syscall overhead ≈ 10 seconds.
glibc makes ~5-10 mmap calls for the same workload.

This is a known architectural limitation of the naive approach, 
not a bug. The fix is pre-allocating large slabs — the basis of 
slab allocation and tcmalloc's design.

## Known Limitations

- mmap per allocation: primary cause of benchmark slowdown
- Singly linked list: O(n) traversal for every malloc call
- Forward-only coalescing: cannot merge with previous block 
  without a doubly linked list
- No compatibility with glibc internals: programs using 
  malloc_usable_size() or SIMD-aligned allocations will crash
