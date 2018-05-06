/* --------------------------------------------------------------------------
 * WORLD		Copyright (c) Simis Ltd 1993,994,1995
 * ---			All Rights Reserved
 *
 * File:		mem.c
 * Ver:			1.00
 * Desc:		Dynamic memory allocator.
 *
 * heap_alloc, heap_free   - general returnable non-optimised heap allocation
 *                           of memory
 *
 * block_alloc, block_free - optimised allocation of small (4-1024 bytes)
 *                           fixed sized blocks
 * release_free_blocks     - returns any cached free blocks to main heap
 * 
 * -------------------------------------------------------------------------- */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "mem.h"

typedef struct Block {
    struct Block *next;
} Block;

#define BLOCK_SIZE 4                              /* Can not be smaller than
												   * sizeof(Block) */
#define INDEX_SHIFT 2
#define MAX_BLOCK_SIZE 1024                       /* Size of maximum block on
												   * free lists in bytes */
#define MAX_BLOCK_INDEX MAX_BLOCK_SIZE/BLOCK_SIZE

#ifdef MemMon
typedef struct {
    int allocated, fromheap, inuse;
} BlockMon;
static BlockMon block_mon[MAX_BLOCK_INDEX+8];
#endif

static Block *free_blocks[MAX_BLOCK_INDEX];
long int heap_allocated = 0L;

/* heap memory allocation - use C library malloc/free */
void *heap_alloc(unsigned nbytes)
{
    void *ptr;

    if (nbytes == 0)
        return NULL;

    /* Ensure nbytes is an integer multiple of sizeof(int) */
    nbytes = (nbytes + (sizeof(int) - 1)) & ~(sizeof(int) - 1);

    heap_allocated += nbytes;
    ptr = malloc(nbytes);

	return ptr;
}

void heap_free(void *ptr)
{
    free(ptr);
}

void *block_alloc(unsigned nbytes)
{
	void *ptr;
    int index;

    if (nbytes == 0)
        return NULL;

    nbytes = (nbytes + (BLOCK_SIZE - 1)) & ~(BLOCK_SIZE - 1);

	if (nbytes > MAX_BLOCK_SIZE) {
		return heap_alloc(nbytes);
	}

	index = (nbytes >> INDEX_SHIFT);
	if (free_blocks[index] == NULL) {
        #ifdef MemMon
            block_mon[index].allocated += 1;
            block_mon[index].fromheap  += 1;
            block_mon[index].inuse     += 1;
        #endif
		return heap_alloc(nbytes);
	}
    ptr = (void*)free_blocks[index];
    free_blocks[index] = free_blocks[index]->next;

#ifdef MemMon
    block_mon[index].allocated += 1;
    block_mon[index].inuse     += 1;
#endif


	return ptr;
}

void block_free(void *ptr, unsigned nbytes)
{
    int index;

    if (nbytes == 0)
        return;

    nbytes = (nbytes + (BLOCK_SIZE - 1)) & ~(BLOCK_SIZE - 1);

	if (nbytes > MAX_BLOCK_SIZE) {
		heap_free(ptr);
        return;
	}

	index = (nbytes >> INDEX_SHIFT);
    ((Block*)ptr)->next = free_blocks[index];
	free_blocks[index] = (Block*)ptr;

#ifdef MemMon
    block_mon[index].inuse -= 1;
#endif
}

static void release_free_blocklist(Block *b)
{
    Block *next;

    while (b) {
        next = b->next;
        heap_free((void*)b);
        b = next;
    }
}

void release_free_blocks(void)
{
    Block *b;
    int i;

    for (i = 0; i < MAX_BLOCK_INDEX; i++) {
        release_free_blocklist(free_blocks[i]);
    }
}

int heap_available(void)
{
    Block *blocks, *b;
    int n;

    n      = 0;
    blocks = NULL;
    while ((b = malloc(1024)) != NULL) {
        n += 1;
        b->next = blocks;
        blocks = b;
    }
    /* Free allocated blocks */
    while (blocks) {
        b = blocks;
        blocks = blocks->next;
        free(b);
    }
    return n;
}

#ifdef MemMon
static int free_length(Block *b)
{
    int n;

    n = 0;
    while (b) {
        n += 1;
        b = b->next;
    }
    return n;
}

void memory_log(void)
{
    int i, ba, bfh, bin, bf;

    debug("\nMEMORY LOG\n");
    debug("==========\n");
    debug("Heap allocated = %d\n", heap_allocated/1024);
    debug("Heap free      = %d\n", heap_available());

    debug("BLOCK ALLOCATION\n");
    debug("blck size> total_allocated fromheap    in_use   free\n");
    ba = bfh = bin = bf = 0;
	for (i = 0; i < MAX_BLOCK_INDEX; i++) {
        if (block_mon[i].allocated > 0)
            debug("%03d>         %8d  %8d  %8d  %8d\n", i*4, block_mon[i].allocated, block_mon[i].fromheap, block_mon[i].inuse, free_length(free_blocks[i]));
        ba  +=  block_mon[i].allocated;
        bfh +=  block_mon[i].fromheap;
        bin +=  block_mon[i].inuse;
        bf  +=  free_length(free_blocks[i]);
	}
    debug("TOTAL        %8d  %8d  %8d  %8d\n", ba, bfh, bin, bf);
}
#endif

/* Backward compatible memory allocation functions */
void *alloc_bytes(unsigned nbytes)
{
    return(heap_alloc(nbytes));
}


/* Memory allocation - performance test
 * ------------------------------------
 */
int mem_rand(int max)
{
    static int seeded = 0;
    int r;

    if (seeded == 0) {
        srand(clock());
        seeded = 1;
    }
    r = rand() / (RAND_MAX / max);
    r = r >> 2;
    return r*4;
}

typedef struct Blk {
    struct Blk *next;
    int    n;
} Blk;

/* Allocate 1000 random blocks 8 - 1024 bytes
 * Deallocate blocks
 * Repeat 100 times
 */
void benchmark_memory_alloc(void)
{
    int i, j, n, t, f, t1, t2;
    struct Blk *b, *b1;

    t = clock();
    f = 0;
    for (i = 0; i < 1000; i++) {
        b = b1 = NULL;
        for (j = 0; j < 100; j++) {
            n = mem_rand(1016)+8;
            b = heap_alloc(n);
            if (b) {
                b->n = n;
                b->next = b1;
                b1 = b;
            } else {
                f += 1;
            }
        }
        while (b1) {
            b = b1;
            b1 = b1->next;
            heap_free(b);
        }
    }
    t1 = clock()-t;

    t = clock();
    f = 0;
    for (i = 0; i < 1000; i++) {
        b = b1 = NULL;
        for (j = 0; j < 100; j++) {
            n = mem_rand(1016)+8;
            b = block_alloc(n);
            if (b) {
                b->n = n;
                b->next = b1;
                b1 = b;
            } else {
                f += 1;
            }
        }
        while (b1) {
            b = b1;
            b1 = b1->next;
            block_free(b, b->n);
        }
    }
    t2 = clock()-t;

    printf("t(cs) = %d %d\n", t1, t2);
}

void init_mem(void)
{
	int i;

	for (i = 0; i < MAX_BLOCK_INDEX; i++) {
		free_blocks[i] = NULL;
	}
    heap_allocated = 0L;

#ifdef MemMon
	for (i = 0; i < MAX_BLOCK_INDEX; i++) {
        block_mon[i].allocated = 0;
        block_mon[i].fromheap  = 0;
        block_mon[i].inuse     = 0;
	}
#endif
}
