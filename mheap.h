#ifndef MHEAP_H
#define MHEAP_H

#include <stdint.h>
#include <stdlib.h>

/* 以N字节对齐 */
#define tBYTE_ALIGNMENT 4

/* 全局静态堆大小 */
#define tMEM_SIZETOALLOC (1024 * 10)

extern void * tAllocHeapforeach(unsigned int sizeToAlloc);
extern void tFreeHeapforeach(void* tObj);

#endif