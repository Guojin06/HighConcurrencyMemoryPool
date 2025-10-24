#pragma once
#include <atomic>
#include "Common.h"
#include "ThreadCache.h"
#include "PageCache.h"


//性能统计初步指标，设计成原子变量，在高频调用且较为简单场景下性能优于锁
std::atomic<size_t> g_allocCount{0};     // 分配次数
std::atomic<size_t> g_freeCount{0};      // 释放次数
std::atomic<size_t> g_currentMemory{0};  // 当前内存（字节）
std::atomic<size_t> g_peakMemory{0};     // 峰值内存（字节）

// 统一对外接口 - 隐藏内部实现细节
// 提供类似malloc/free的简洁接口

// 统一分配接口
static inline void* ConcurrentAlloc(size_t size)
{
    // 性能统计：记录分配
    g_allocCount++;
    g_currentMemory += size;
    
    // 更新峰值内存（多线程环境下可能有小误差，但统计场景可接受）
    size_t current = g_currentMemory.load();
    size_t peak = g_peakMemory.load();
    if (current > peak) {
        g_peakMemory.store(current);
    }
    
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
    // 性能统计：记录释放
    g_freeCount++;
    g_currentMemory -= size;
    
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