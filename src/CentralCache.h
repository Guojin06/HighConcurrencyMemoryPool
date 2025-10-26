#ifndef __CENTRAL_CACHE_H__
#define __CENTRAL_CACHE_H__

#include "Common.h"
#include <unordered_map>
#include <mutex>

//优化点1，对齐到缓存行，避免伪共享
struct alignas(64) PaddedMutex {
    std::mutex mtx;
    char padding[64 - sizeof(std::mutex)];
};

// 中心缓存 - 单例模式
// 负责从PageCache获取Span，切分成小对象供ThreadCache使用
class CentralCache {
public:
    // 获取单例对象
    static CentralCache* GetInstance() {
        static CentralCache instance;
        return &instance;
    }
    
    // 从中心缓存获取一定数量的对象给ThreadCache
    // start: 返回获取对象链表的起始指针
    // end: 返回获取对象链表的结束指针
    // size: 对象大小
    // num: 期望获取的对象数量
    // 返回值: 实际获取到的对象数量
    size_t FetchRangeObj(void*& start, void*& end, size_t size, int num);
    
    // 将一定数量的对象释放回Span
    // start: 要释放的对象链表头
    // size: 对象大小
    void ReleaseListToSpans(void* start, size_t size);

private:
    CentralCache() {}  // 构造函数私有化
    CentralCache(const CentralCache&) = delete;  // 禁止拷贝构造
    CentralCache& operator=(const CentralCache&) = delete;  // 禁止赋值
    
    SpanList _spanLists[208];  // 按对象大小映射的Span双向链表数组
    std::unordered_map<PAGE_ID, Span*> _pageToSpan;  // 页号到Span的映射
    PaddedMutex _mtx[208];  // 全局锁，保护CentralCache的并发访问,细粒度化改进
};

#endif

