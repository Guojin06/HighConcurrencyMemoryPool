#pragma once
#include "Common.h"

class ThreadCache
{
public:
    // 申请和释放内存对象
    void* Allocate(size_t size)
    {
        //1.计算索引
        size_t index = SizeClass::Index(size);
        //2.检查对应的Freelist是否为空
        if(!_freeLists[index].Empty())
        {
            //3.有内存直接返回
            return _freeLists[index].Pop();
        }
        //没内存了，向CentralCache批量申请
        return FetchFromCentralCache(index, SizeClass::RoundUp(size));
    };
    void Deallocate(void* ptr, size_t size)
    {
        //1.计算索引，和Allocate一样
        size_t index = SizeClass::Index(size);
        //2.将对象push到对应的Freelist中
        _freeLists[index].Push(ptr);
    };

private:
    // 向CentralCache批量申请内存对象
    void* FetchFromCentralCache(size_t index, size_t size)
    {
        // TODO: 实现慢增长算法，当前固定申请8个
        // TODO: 调用真正的CentralCache，当前用malloc模拟
        
        // 前7个直接Push到FreeList缓存起来
        for (int i = 0; i < 7; ++i) {
            void* obj = malloc(size);
            _freeLists[index].Push(obj);
        }
        
        // 第8个直接返回给用户使用
        void* returnObj = malloc(size);
        return returnObj;
    }
    FreeList _freeLists[NFREELIST];  // 自由链表数组
};

// 通过TLS 每个线程无锁的获取自己的专属的ThreadCache对象
static thread_local ThreadCache* pTLSThreadCache = nullptr;

// 获取当前线程的ThreadCache对象
static ThreadCache* GetTLSThreadCache() {
    if (pTLSThreadCache == nullptr) {
        pTLSThreadCache = new ThreadCache;
    }
    return pTLSThreadCache;
}
