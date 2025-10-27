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
            ReleaseToCentralCache(index,size);
        }
    };

private:
    // 性能调优：提高缓存阈值，减少触发释放频率
    // NumMoveSize上限512，阈值设为1536（3倍）
    // 这样拿3次（512×3=1536）才触发释放，减少锁竞争
    static const size_t SMALL_OBJ_MAX_COUNT = 1536;   // 小对象最大缓存数（原512）
    static const size_t MEDIUM_OBJ_MAX_COUNT = 768;   // 中等对象最大缓存数（原256）
    static const size_t LARGE_OBJ_MAX_COUNT = 192;    // 大对象最大缓存数（原64）

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
        
        
        void* start = nullptr;
        void* end = nullptr;
        
        // 从CentralCache批量获取对象，获取实际数量
        size_t batchNum =  SizeClass::NumMoveSize(size);
        size_t actualNum = CentralCache::GetInstance()->FetchRangeObj(start, end, size, batchNum);
        
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
    void ReleaseToCentralCache(size_t index, size_t size)
    {
        //步骤1.计算归还个数
        size_t releaseNum = _freeLists[index].Size() >> 1;//一半
        //步骤2.从FreeList弹出releaseNum个对象
        void* start = nullptr;
        void* end = nullptr;
        _freeLists[index].PopRange(start, end, releaseNum);//调用PopRange函数，从FreeList批量弹出releaseNum个对象
        //步骤3.计算对象大小
        //这里选择直接传入size，省去从索引计算size的步骤，我们没有实现从索引计算size的函数，这里直接传入size，也提高了效率
        //步骤4.调用CentralCache::ReleaseListToSpans()将对象链表返回给CentralCache
        CentralCache::GetInstance()->ReleaseListToSpans(start, size);
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
