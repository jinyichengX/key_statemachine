/******************************************************************************************
* @file         : mheap.c
* @Description  : Allocate memory for usr objects in constant fix memory.
* @autor        : Jinyicheng
* @emil:        : 2907487307@qq.com
* @version      : 1.0
* @date         : 2023/8/14    
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2023/08/14	    V1.0	  jinyicheng	      创建
 * 2023/08/15       V1.0      jinyicheng          创建
 * 2023/08/31       V1.0      jinyicheng          创建
 * ******************************************************************************************/
#include "mheap.h"
#include <stddef.h>
#include <stdbool.h>

#if tBYTE_ALIGNMENT == 32
#define tBYTE_ALIGNMENT_MASK    ( 0x001f )
#elif tBYTE_ALIGNMENT == 16
#define tBYTE_ALIGNMENT_MASK    ( 0x000f )
#elif tBYTE_ALIGNMENT == 8
#define tBYTE_ALIGNMENT_MASK    ( 0x0007 )
#elif tBYTE_ALIGNMENT == 4
#define tBYTE_ALIGNMENT_MASK    ( 0x0003 )
#elif tBYTE_ALIGNMENT == 2
#define tBYTE_ALIGNMENT_MASK    ( 0x0001 )
#elif tBYTE_ALIGNMENT == 1
#define tBYTE_ALIGNMENT_MASK    ( 0x0000 )
#endif

typedef struct stBLOCKBLINK
{
    /* 指向下一个对象block首地址 */
    struct stBLOCKBLINK* pNextBlockLinkStruct;
    /* 对象空间大小 */
    unsigned int AllocSize;
}BlockLink_t, * pBlockLink;

/* 校验是否以规定字节对齐 */
#define BlkAssertAligned(size) if(0 != (size&tBYTE_ALIGNMENT_MASK)) { return NULL; }

/* 头尾Block节点AllocSize值为0 */
BlockLink_t* ObjStartBlock, * ObjEndBlock;

static int maxRemainingSize = 0;
static int ObjAllocated = 0;

/* 定义全局静态堆 */
static unsigned char theap[tMEM_SIZETOALLOC];

/* BlockLink结构体所需要分配的堆大小（向上作字节对齐） */
static const unsigned int BlockLinkStructSize = (sizeof(BlockLink_t) + ((unsigned int)(tBYTE_ALIGNMENT - 1))) & ~((unsigned int)tBYTE_ALIGNMENT_MASK);

/**********************************************************************
 * 函数名称： tInitializeHeap
 * 功能描述： 初始化静态堆
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 堆内存首地址
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2023/08/14	    V1.0	  jinyicheng	      创建
 ***********************************************************************/
static void* tInitializeHeap(void)
{
    unsigned char* heapInv;
    unsigned int heapSizeLeft;
    unsigned int pTemp;

    /* 保存堆内存首地址 */
    unsigned int t_HeapBottom = (unsigned int)theap;

    /* 向上作字节对齐 */
    if (t_HeapBottom & tBYTE_ALIGNMENT_MASK)
        t_HeapBottom = t_HeapBottom - (t_HeapBottom & tBYTE_ALIGNMENT_MASK) + tBYTE_ALIGNMENT;

    /* 保存字节对齐后的地址，作为堆的首地址 */
    heapInv = (unsigned char*)t_HeapBottom;

    /* 计算Heap剩余空间 */
    heapSizeLeft = tMEM_SIZETOALLOC - (t_HeapBottom - (unsigned int)theap);

    /* 在堆内存尾部插入BlockLink节点*/
    /* 1.先对该节点所在地址作字节对齐 */
    pTemp = (unsigned int)(heapInv + heapSizeLeft - BlockLinkStructSize);
    pTemp &= ~((unsigned int)tBYTE_ALIGNMENT_MASK);//向下作字节对齐

    /* 2.对首位链表地址之差进行字节对齐校验 */
    BlkAssertAligned((pTemp - (unsigned int)heapInv));

    /* 3.填充堆内存尾部的BlockBlink节点 */
    ObjEndBlock = (pBlockLink)pTemp;
    ObjEndBlock->AllocSize = 0;
    ObjStartBlock = NULL;

    /* 在堆内存首部插入BlockLink节点 */
    ObjStartBlock = (pBlockLink)heapInv;
    ObjStartBlock->pNextBlockLinkStruct = ObjEndBlock;
    ObjStartBlock->AllocSize = 0;

    /* 将首结点下一个成员赋值为尾节点，形成单向循环链表 */
    ObjEndBlock->pNextBlockLinkStruct = ObjStartBlock;

    maxRemainingSize = pTemp - (unsigned int)heapInv;
    BlkAssertAligned(maxRemainingSize);

    /* 返回可用堆内存首地址 */
    return (void*)heapInv;
}

/**********************************************************************
 * 函数名称： tAllocHeap
 * 功能描述： 从静态内存为用户对象动态分配空间
 * 输入参数： 无
 * 输出参数： 无
 * 返 回 值： 返回用户对象句柄
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2023/08/14	    V1.0	  jinyicheng	      创建
 ***********************************************************************/
static void* tAllocHeap(unsigned int sizeToAlloc)
{
    BlockLink_t * pObjBlock = NULL, * pObjBlkInd = ObjStartBlock, * pToInsert = NULL;
    unsigned int BlockRemaining = 0;
    unsigned int MinimumSize = 0;
    int block_diff;
    bool MinFound = false;
    BlockLink_t * pToInsertLastBlk = NULL;

    /* 检查堆是否已被初始化 */
    if (ObjEndBlock == NULL) {
        if (NULL == tInitializeHeap()) {
            return NULL;
        }
    }

    /* 传参校验，计算所需堆大小 */
    if (sizeToAlloc > 0)
    {
        /* 总分配空间大小等于用户所需空间加BlockLink结构体所需要分配的堆大小，BlockLink需单独作字节对齐 */
        sizeToAlloc = sizeToAlloc + BlockLinkStructSize;

        /* 将分配到的总空间继续向上作对齐 */
        if (0x00 != (sizeToAlloc & tBYTE_ALIGNMENT_MASK))
        {
            if (sizeToAlloc + (sizeToAlloc - (sizeToAlloc & tBYTE_ALIGNMENT_MASK) + tBYTE_ALIGNMENT)
                > sizeToAlloc)
            {
                sizeToAlloc = sizeToAlloc - (sizeToAlloc & tBYTE_ALIGNMENT_MASK) + tBYTE_ALIGNMENT;
                BlkAssertAligned(sizeToAlloc);
            }
            else
            {
                sizeToAlloc = 0;
            }
        }
    }
    else {
        return NULL;
    }

    if (sizeToAlloc) {
        /* 在堆中初始化BlockLink，并插入到列表中 */
        /* 1.添加第一个对象 */
        if (
            (ObjAllocated == 0) && \
            (sizeToAlloc <= (((unsigned int)ObjStartBlock->pNextBlockLinkStruct)-((unsigned int)ObjStartBlock)-BlockLinkStructSize))
            )
        {
            pObjBlock = (unsigned int)ObjStartBlock + BlockLinkStructSize;
            (BlockLink_t *)pObjBlock;
            pObjBlock->AllocSize = sizeToAlloc - BlockLinkStructSize;

            /* 将该Block节点插入链表 */
            ObjStartBlock->pNextBlockLinkStruct = (BlockLink_t *)pObjBlock;
            pObjBlock->pNextBlockLinkStruct = ObjEndBlock;

            ObjAllocated ++;

            /* 返回对象句柄 */
            return (void *)((unsigned int)ObjStartBlock + BlockLinkStructSize + BlockLinkStructSize);
        }

        /* 2.添加第n(n != 1)对象 */
        MinimumSize = (unsigned int)pObjBlkInd->pNextBlockLinkStruct - (unsigned int)pObjBlkInd\
            - BlockLinkStructSize - pObjBlkInd->AllocSize;

        /* 遍历链表，选择适合插入的位置 */
        for (pObjBlkInd = ObjStartBlock; pObjBlkInd < ObjEndBlock; pObjBlkInd = pObjBlkInd->pNextBlockLinkStruct)
        {
            block_diff = (unsigned int)pObjBlkInd->pNextBlockLinkStruct - (unsigned int)pObjBlkInd\
                - BlockLinkStructSize - pObjBlkInd->AllocSize;
            if (block_diff > 0) {
                MinimumSize = block_diff; MinFound = true;
            }
            /* 找出最小适合插入对象的堆空间 */
            if (true == MinFound) {
                if ((block_diff <= MinimumSize) && (block_diff > 0)\
                    && (block_diff >= sizeToAlloc)) {
                    MinimumSize = block_diff;

                    /* 记录适合插入该节点所在位置的上一节点 */
                    pToInsertLastBlk = pObjBlkInd;

                    /* 找出适合插入节点的地址 */
                    pToInsert = (pBlockLink)(((unsigned int)pObjBlkInd->pNextBlockLinkStruct) - MinimumSize);
                }
            }
        }
        if (MinimumSize < sizeToAlloc) return NULL;

        BlkAssertAligned((unsigned int)pToInsert);

        /* 赋值该节点对象所占空间大小 */
        pToInsert->AllocSize = sizeToAlloc - BlockLinkStructSize;

        /* 将该Block节点插入链表 */
        pToInsert->pNextBlockLinkStruct = pToInsertLastBlk->pNextBlockLinkStruct;
        pToInsertLastBlk->pNextBlockLinkStruct = pToInsert;

        ObjAllocated += 1;

        /* 返回对象句柄 */
        return (void *)((unsigned int)pToInsert + BlockLinkStructSize);
    }

    return NULL;
}

/**********************************************************************
 * 函数名称： tTakeBlockPos
 * 功能描述： 从单向循环链表中寻找此节点所在的位置
 * 输入参数： Block 此节点
 * 输出参数： 无
 * 返 回 值： 上一节点句柄
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2023/08/15	    V1.0	  jinyicheng	      创建
 ***********************************************************************/
static void* tTakeBlockPos(BlockLink_t * Block)
{
    pBlockLink pObjBlkInd, pObjToFree;

    pObjToFree = Block;
    pObjBlkInd = ObjStartBlock;

    /* 遍历链表中的Block节点 */
    for (;pObjBlkInd <= ObjEndBlock;pObjBlkInd = pObjBlkInd->pNextBlockLinkStruct)
    {
        /* 若此节点的下一个节点是传入的Block，则返回当前节点 */
        if (pObjBlkInd->pNextBlockLinkStruct == Block)
        {
            return pObjBlkInd;
        }
    }
    return NULL;
}

/**********************************************************************
 * 函数名称： tFreeHeap
 * 功能描述： 从堆内存释放用户对象
 * 输入参数： tObj 对象句柄
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2023/08/15	    V1.0	  jinyicheng	      创建
 ***********************************************************************/
static void tFreeHeap(void* tObj)
{
    pBlockLink pBlockToFree = NULL;
    pBlockLink pBlockToFreeLast = NULL;

    /* 传参校验 */
    if (NULL == tObj)
        return;

    /* 获得对象句柄所在Block节点首地址 */
    pBlockToFree = (pBlockLink)((unsigned int)tObj - BlockLinkStructSize);

    /* 将该Block节点从链表移除 */
    /* 1.先找出此节点的上一个节点 */
    pBlockToFreeLast = (pBlockLink)tTakeBlockPos(pBlockToFree);
    /* 2.将此节点的上一个节点指向此节点的下一个节点 */
    pBlockToFreeLast->pNextBlockLinkStruct = pBlockToFree->pNextBlockLinkStruct;

    ObjAllocated -= 1;
}

/**********************************************************************
 * 函数名称： tFreeHeap
 * 功能描述： 从堆内存开辟空间
 * 输入参数： 大小
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2023/08/31	    V1.0	  jinyicheng	      创建
 ***********************************************************************/
void * tAllocHeapforeach(unsigned int sizeToAlloc)
{
    void * firstaddr = NULL;

    /* 由系统为对象分配空间 */
    firstaddr = malloc(sizeToAlloc);
    
    /* 将对象分配在bss段 */
    if(NULL == firstaddr)
    {
        firstaddr = tAllocHeap(sizeToAlloc);
        return firstaddr;
    }
    else
    {
        return firstaddr;
    }
}

/**********************************************************************
 * 函数名称： tFreeHeapforeach
 * 功能描述： 从堆内存回收空间
 * 输入参数： tObj 对象句柄
 * 输出参数： 无
 * 返 回 值： 无
 * 修改日期        版本号     修改人	      修改内容
 * -----------------------------------------------
 * 2023/08/31	    V1.0	  jinyicheng	      创建
 ***********************************************************************/
void tFreeHeapforeach(void* tObj)
{
    /* 传参校验 */
    if (NULL == tObj)
        return;

    /* 若对象被分配在bss段 */
    if(
        ( (unsigned int)tObj >= (unsigned int)theap ) || \
        ( (unsigned int)tObj <= ((unsigned int)theap+tMEM_SIZETOALLOC) )
        )
    {
        tFreeHeap(tObj);
        return;
    }
    /* 若对象由系统分配 */
    else
    {
        free(tObj);
    }
}