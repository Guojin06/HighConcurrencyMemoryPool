#pragma once

#include "Common.h"
#include <unordered_map>
#include <mutex>
class PageCache{//单例模式
public:
    static PageCache* GetInstance(){
        static PageCache _sInst;
        return &_sInst;
    }
    //接口一：当CentralCache没有内存时，向PageCache申请内存
    Span* NewSpan(size_t k);//参数，需要多少页，k=页数
    //接口二：当ThreadCache释放内存时，向PageCache释放内存
    void ReleaseSpanToPageCache(Span* span);//参数，要释放的Span

private:
    PageCache(){}//构造函数私有化防止外部构造
    PageCache(const PageCache&)=delete;//禁止拷贝构造
    PageCache& operator=(const PageCache&)=delete;//禁止赋值
    SpanList _spanLists[NPAGES];//按页大小映射的Span双向链表数组
    std::mutex _pageMtx;//全局锁，保护PageCache的并发访问
    std::unordered_map<PAGE_ID, Span*> _pageToSpan;//页号到Span的映射
};