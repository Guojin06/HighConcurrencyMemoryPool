#pragma once
#include "Common.h"
#include "CentralCache.h"

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
        
        //3.检查是否需要批量归还给CentralCache
        if (ListTooLong(index)) {
            // TODO: 实现批量归还给CentralCache的逻辑
            // ReleaseToCentralCache(index);
        }
    };

private:
    // TODO: 性能调优时根据实测数据调整这些阈值
    static const size_t SMALL_OBJ_MAX_COUNT = 512;    // 小对象最大缓存数
    static const size_t MEDIUM_OBJ_MAX_COUNT = 256;   // 中等对象最大缓存数  
    static const size_t LARGE_OBJ_MAX_COUNT = 64;     // 大对象最大缓存数

    // 检查FreeList是否过长，需要批量归还
    bool ListTooLong(size_t index)
    {
        size_t maxCount = 0;
        
        if (index <= 15) {        // 8-128字节，小对象
            maxCount = SMALL_OBJ_MAX_COUNT;
        } 
        else if (index <= 71) {   // 129-1024字节，中等对象  
            maxCount = MEDIUM_OBJ_MAX_COUNT;
        }
        else {                    // 更大的对象
            maxCount = LARGE_OBJ_MAX_COUNT;
        }
        
        return _freeLists[index].Size() > maxCount;
    }

    // 向CentralCache批量申请内存对象
    void* FetchFromCentralCache(size_t index, size_t size)
    {
        // TODO: 实现慢增长算法，当前固定申请8个
        
        void* start = nullptr;
        void* end = nullptr;
        
        // 从CentralCache批量获取对象，获取实际数量
        size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, size, 8);
        
        // 把前actualNum-1个Push到FreeList缓存
        void* cur = start;
        for (size_t i = 0; i < actualNum - 1; ++i) {
            void* next = NextObj(cur);
            _freeLists[index].Push(cur);
            cur = next;
        }
        
        // 返回最后一个对象给用户
        return cur;
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
