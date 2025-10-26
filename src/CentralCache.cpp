#include "CentralCache.h"
#include "PageCache.h"

// 从CentralCache获取一批对象
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t size, int num) {
    size_t index = SizeClass::Index(size);
    
    // 加锁保护
    _mtx[index].mtx.lock();
    
    // 1. 先从对应的SpanList中找有空闲对象的Span
    Span* span = _spanLists[index].Begin();
    while (span != _spanLists[index].End()) {
        if (span->_freeList != nullptr) {
            break;  // 找到有空闲对象的Span
        }
        span = span->_next;
    }
    
    // 2. 如果没找到，向PageCache申请新的Span
    if (span == _spanLists[index].End()) {
        _mtx[index].mtx.unlock();  // 先解锁，避免死锁
        
        // 向PageCache申请Span
        size_t numPages = SizeClass::NumMovePage(size);
        span = PageCache::GetInstance()->NewSpan(numPages);
        
        // 切分Span成小块对象
        size_t spanBytes = span->_n << PAGE_SHIFT;  // Span总字节数（页数 * 8KB）
        size_t blockCount = spanBytes / size;  // 能切多少块
        void* spanStart = (void*)((span->_pageId) << PAGE_SHIFT);  // Span起始地址
        
        // 串成链表：前blockCount-1块
        for (size_t i = 0; i < blockCount - 1; ++i) {
            void* cur = (void*)((char*)spanStart + i * size);
            void* next = (void*)((char*)spanStart + (i + 1) * size);
            NextObj(cur) = next;
        }
        // 最后一块指向nullptr
        void* last = (void*)((char*)spanStart + (blockCount - 1) * size);
        NextObj(last) = nullptr;
        
        span->_freeList = spanStart;  // 链表头
        span->_objSize = size;         // 记录对象大小
        span->_isUse = true;
        
        _mtx[index].mtx.lock();  // 重新加锁
        _spanLists[index].PushFront(span);  // 挂到SpanList
        // _pageToSpan[span->_pageId] = span;  // 建立页号→Span映射
        //这里只映射了首页地址，若Span管理多页，后面页没有建立映射，如果返回对象来自后面页，这会造成ThreadCache无法找到Span，导致内存泄露
        //正确做法：
        //建立每一页的映射（用于释放时根据对象地址查找Span）
        for(PAGE_ID  i = 0 ; i< span->_n;++i)
        {
            _pageToSpan[span->_pageId+i] = span;
        }
    }
    
    // 3. 从Span的freeList中取出num个对象
    void* cur = span->_freeList;
    void* prev = nullptr;
    size_t actualNum = 0;
    
    for (int i = 0; i < num && cur != nullptr; ++i) {
        prev = cur;
        cur = NextObj(cur);
        ++actualNum;
    }
    
    // 更新返回值
    start = span->_freeList;
    end = prev;
    
    // 更新Span的freeList
    span->_freeList = cur;
    span->_useCount += actualNum;
    
    _mtx[index].mtx.unlock();
    
    return actualNum;
}

// 将对象链表释放回CentralCache
void CentralCache::ReleaseListToSpans(void* start, size_t size) {
    size_t index = SizeClass::Index(size);
    

    _mtx[index].mtx.lock();  // 加锁保护
    
    // 遍历要释放的对象链表
    while (start != nullptr) {
        void* next = NextObj(start);  // 先保存下一个节点
        
        // 1. 根据地址计算页号，找到对应的Span
        PAGE_ID pageId = ((PAGE_ID)start) >> PAGE_SHIFT;
        Span* span = _pageToSpan[pageId];
        
        // 2. 把对象还回Span的freeList（头插法）
        NextObj(start) = span->_freeList;
        span->_freeList = start;
        
        // 3. 更新使用计数
        span->_useCount--;
        
        // 4. 如果Span的所有对象都释放了，归还给PageCache
        if (span->_useCount == 0) {
            // 4.1 从SpanList中摘除
            _spanLists[index].Erase(span);
            
            // 4.2 删除CentralCache的映射（每一页都要删除）
            for (PAGE_ID i = 0; i < span->_n; ++i) {
                _pageToSpan.erase(span->_pageId + i);
            }
            
            // 4.3 设置为未使用状态
            span->_isUse = false;
            
            // 4.4 先解锁，避免与PageCache的锁形成死锁
            _mtx[index].mtx.unlock();
            
            // 4.5 归还给PageCache（PageCache会进行页合并）
            PageCache::GetInstance()->ReleaseSpanToPageCache(span);
            
            // 4.6 重新加锁，因为while循环还要继续
            _mtx[index].mtx.lock();
        }
        
        // 5. 移动到下一个对象
        start = next;
    }
    
    _mtx[index].mtx.unlock();
}

