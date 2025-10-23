#pragma once
#include "Common.h"
#include "ThreadCache.h"
#include "PageCache.h"

// 统一对外接口 - 隐藏内部实现细节
// 提供类似malloc/free的简洁接口

// 统一分配接口
static inline void* ConcurrentAlloc(size_t size)
{
    // 大内存（>256KB）直接走malloc，不使用内存池
    // 原因：大内存走内存池效率低，且占用资源
    if (size > MAX_BYTES)
    {
        return malloc(size);
    }
    else
    {
        // 小内存走三层缓存架构
        return GetTLSThreadCache()->Allocate(size);
    }
}

// 统一释放接口
static inline void ConcurrentFree(void* ptr, size_t size)
{
    // 根据size判断是大内存还是小内存
    if (size > MAX_BYTES)
    {
        // 大内存直接free
        free(ptr);
    }
    else
    {
        // 小内存归还给内存池
        GetTLSThreadCache()->Deallocate(ptr, size);
    }
}