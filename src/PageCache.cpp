#include "PageCache.h"
#include "Common.h"

//实现newSpan函数
Span* PageCache::NewSpan(size_t k){
    _pageMtx.lock();
    //参数检查：1.如果申请的页数大于128页，则直接调用系统接口分配内存
    if(k > 128){
        //1.直接向OS申请K页
        void* ptr = SystemAlloc(k);
        //2.创建一个新的Span对象
        Span* span = new Span;
        //3.设置Span的页号以及页数
        span->_pageId = ((PAGE_ID)ptr) >> PAGE_SHIFT;//将申请到的内存起始地址转换为页号
        span->_n = k;
        // //4.将Span对象插入到_spanLists数组中,错误！这里超大页，不需要插入到_spanLists数组中
        // _spanLists[k].PushFront(span);
        //5.返回Span对象
        _pageMtx.unlock();//这里要解锁，因为NewSpan函数是公共接口，可能被多个线程同时调用
        return span;
    }
    //2.继续处理正常情况K<=128，注意索引对应，_spanLists[k-1]表示k页的Span链表
    else if(k <= 128 && k > 0){
        //1.先检查k页的Span链表是否为空
        if(!_spanLists[k-1].Empty()){
            //2.如果链表不为空，则从链表中获取一个Span
            Span* kSpan = _spanLists[k-1].PopFront();
            //3.建立页号到Span的映射（用于后续页合并）
            for(size_t i = 0; i < kSpan->_n; ++i){
                _pageToSpan[kSpan->_pageId + i] = kSpan;
            }
            //4.解锁并返回Span对象
            _pageMtx.unlock();
            return kSpan;
        }
        //4.如果链表为空，则继续处理
        else{
            //1.如果链表为空，则继续处理
            //2.从_spanLists[k]开始，找到第一个不为空的Span链表
            for(size_t i = k; i < 128; ++i){
                if(!_spanLists[i].Empty()){
                    //3.如果找到不为空的Span链表，则从链表中获取一个Span
                    // Span* nSpan = _spanLists[i].PopFront();
                    // //4.返回Span对象
                    // return nSpan;这种写法错误
                    //这里直接返回不对，比如要3页找到了一个5页，应该把5页的Span切分成3页和2页，3页的Span返回，2页的Span继续处理
                    Span* nSpan = _spanLists[i].PopFront();//获取找到的大页，要切分
                    Span* kSpan = new Span;
                    kSpan->_pageId = nSpan->_pageId;
                    kSpan->_n = k;//设置切分后的小页的kSpan的页号以及页数
                    nSpan->_pageId += k;//切分后的大页的页号加上k，指向新的大页的起始页号
                    nSpan->_n -= k;//切分后的大页的页数减去k，指向新的大页的剩余页数
                    _spanLists[nSpan->_n - 1].PushFront(nSpan);//切分后的小页插入到对应大小的Span链表中
                    //建立kSpan每一页的映射
                    for(size_t j = 0; j < kSpan->_n; ++j){
                        _pageToSpan[kSpan->_pageId + j] = kSpan;
                    }
                    _pageMtx.unlock();
                    return kSpan;
                }
                // else{
                //     //5.如果找到最后一个Span链表，则继续处理，向OS申请128页
                //     void* ptr = SystemAlloc(128);
                //     //6.创建一个新的Span对象
                //     Span* span = new Span;
                //     //7.设置Span的页号以及页数
                //     span->_pageId = ((PAGE_ID)ptr) >> PAGE_SHIFT;
                //     span->_n = 128;
                //     //8.将Span对象插入到_spanLists数组中
                //     _spanLists[128].PushFront(span);
                //     //9.返回Span对象
                //     return span;
                // }不应该在循环体内处理，应该在循环体外处理，因为每次有一个空就下来申请一次这逻辑错了
            }
            // return nullptr;
            //这里应该继续处理，向OS申请128页
            void* ptr = SystemAlloc(128);
            Span* nspan = new Span;//new一个128页的Span对象
            nspan->_pageId = ((PAGE_ID)ptr) >> PAGE_SHIFT;
            nspan->_n = 128;
            //这里仍然需要切分
            Span* kSpan = new Span;//new一个k页的Span对象
            kSpan->_pageId = nspan->_pageId;
            kSpan->_n = k;//设置切分后的小页的kSpan的页号以及页数
            nspan->_pageId += k;//切分后的大页的页号加上k，指向新的大页的起始页号
            nspan->_n -= k;//切分后的大页的页数减去k，指向新的大页的剩余页数
            _spanLists[nspan->_n - 1].PushFront(nspan);//切分后的小页插入到对应大小的Span链表中
            //建立kSpan每一页的映射
            for(size_t j = 0; j < kSpan->_n; ++j){
                _pageToSpan[kSpan->_pageId + j] = kSpan;
            }
            _pageMtx.unlock();
            return kSpan;
        }
    }
    //3.如果k<=0，则返回nullptr
    else{
        _pageMtx.unlock();
        return nullptr;
    }
}
//实现ReleaseSpanToPageCache函数
void PageCache::ReleaseSpanToPageCache(Span* span){
}