#ifndef mem_h
#define mem_h

#include <stdarg.h>

extern void stop(char *format, ...);
extern void debug(char *format, ...);

/* If MemMon is defined then active memory monitoring is turned on
 */
#define MemMon
#ifdef MemMon
/* Write log of memory useage to disk log */
extern void memory_log(void);
#endif

/* Historical memory allocation function */
extern void *alloc_bytes(unsigned nbytes);

/* Main heap allocation */
extern void *heap_alloc(unsigned nbytes);
extern void  heap_free(void *ptr);
extern int heap_available(void);

/* Optimised small block allocation */
extern void *block_alloc(unsigned nbytes);
extern void  block_free(void *ptr, unsigned nbytes);
extern void  release_free_blocks(void);

extern void init_mem(void);
#endif
