#pragma once

#include <iostream>
#include <thread>
#include <mutex>
#include <assert.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
#endif

using std::cout;
using std::endl;

// 全局常量
static const size_t MAX_BYTES = 256 * 1024;  // ThreadCache最大管理256KB
static const size_t NFREELIST = 208;          // 自由链表数组长度
static const size_t NPAGES = 129;             // PageCache最多管理128页
static const size_t PAGE_SHIFT = 13;          // 页大小8KB (2^13)

// 向操作系统申请内存
inline static void* SystemAlloc(size_t kpage) {
    void* ptr = nullptr;
    
#ifdef _WIN32
    ptr = VirtualAlloc(0, kpage << PAGE_SHIFT, 
                       MEM_COMMIT | MEM_RESERVE, 
                       PAGE_READWRITE);
#else
    // Linux版本后面再说
#endif

    if (ptr == nullptr)
        throw std::bad_alloc();
    
    return ptr;
}

// 向操作系统释放内存  
inline static void SystemFree(void* ptr) {
#ifdef _WIN32
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    // Linux版本后面再说  
#endif
}



//获取/设置对象的下一个节点
static void*& NextObj(void* obj)//这个函数可以获取下一个节点，还可以设置下一个节点
{
    return *(void**)obj;
}

class FreeList {
    public:
        void Push(void* obj)//头插，O(1)
        {
            assert(obj);//确保obj不为空
            NextObj(obj) = _freeList;//设置下一个节点为链表头,这里_freeList不是一个节点，是一个指针，存储的是第一个节点的地址
            _freeList = obj;//更新链表头
            _size++;
        };    // 插入
        void* Pop()//头删，不需要参数
        {
            assert(_freeList);//确保链表不为空
            void* obj = _freeList;//保存头结点
            _freeList = NextObj(_freeList);//头指针后移
            _size--;
            return obj;
        };             // 弹出
        bool Empty()
        {
            return _freeList == nullptr;
        };            // 是否为空
        size_t Size()
        {
            return _size;
        };           // 长度
    
    private:
        void* _freeList = nullptr;  // 链表头指针
        size_t _size = 0;           // 当前长度
    };

class SizeClass {//内存对齐+索引计算
    public:
        // 辅助函数：对齐计算（用公式法）
        static inline size_t _RoundUp(size_t bytes, size_t alignNum) {

            return ((bytes + alignNum - 1) & ~(alignNum - 1));//核心掩码对齐算法
        }
        
        // 主函数：根据大小选择对齐数
        static inline size_t RoundUp(size_t bytes) {
            // TODO: 根据size范围，调用_RoundUp
            if (bytes <= 128) {
                return _RoundUp(bytes, 8);      // ← 用8对齐
            }
            else if (bytes <= 1024) {
                return _RoundUp(bytes, 16);     // ← 用16对齐
            }
            else if (bytes <= 8 * 1024) {
                return _RoundUp(bytes, 128);    // ← 用128对齐
            }
            else if (bytes <= 64 * 1024) {
                return _RoundUp(bytes, 1024);   // ← 用1024对齐
            }
            else if (bytes <= 256 * 1024) {
                return _RoundUp(bytes, 8 * 1024); // ← 用8KB对齐
            }
            else {
                // 超过256KB的情况（理论上不会到达这里）
                return _RoundUp(bytes, 1 << PAGE_SHIFT); // 按页对齐
            }
        }
        
        // 辅助函数：计算某个范围内的索引
        static inline size_t _Index(size_t bytes, size_t align_shift) {
            return ((bytes + (1 << align_shift) - 1) >> align_shift) - 1;
        }
        
        // 主函数：计算全局索引
        static inline size_t Index(size_t bytes) {
            assert(bytes <= MAX_BYTES);
            
            // 每个范围占用的索引数量
            static int group_array[4] = {16, 56, 56, 56};
            
            if (bytes <= 128) {
                return _Index(bytes, 3);  // 3表示右移3位，相当于除以8
            }
            else if (bytes <= 1024) {
                return _Index(bytes - 128, 4) + group_array[0];
            }
            else if (bytes <= 8 * 1024) {
                return _Index(bytes - 1024, 7) + group_array[1] + group_array[0];
            }
            else if (bytes <= 64 * 1024) {
                return _Index(bytes - 8 * 1024, 10) + group_array[2] + group_array[1] + group_array[0];
            }
            else if (bytes <= 256 * 1024) {
                return _Index(bytes - 64 * 1024, 13) + group_array[3] + group_array[2] + group_array[1] + group_array[0];
            }
            
            assert(false);
            return -1;
        }
    };

// 页号类型定义
#ifdef _WIN64
    typedef unsigned long long PAGE_ID;
#else
    typedef size_t PAGE_ID;
#endif

struct Span {
    PAGE_ID _pageId = 0;         // 起始页号
    size_t _n = 0;               // 页数
    
    Span* _next = nullptr;       // 双向链表指针
    Span* _prev = nullptr;
    
    size_t _objSize = 0;         // 切分的对象大小（8字节？16字节？）
    size_t _useCount = 0;        // 已分配出去的对象数量
    void* _freeList = nullptr;   // 剩余对象的自由链表
    
    bool _isUse = false;         // 是否正在被CentralCache使用
};
class SpanList {
    public:
        SpanList() {
            _head = new Span;           // 哨兵节点
            _head->_next = _head;       // 循环链表
            _head->_prev = _head;
        }
        
        Span* Begin() { return _head->_next; }
        Span* End() { return _head; }
        bool Empty() { return _head->_next == _head; }
        
        void PushFront(Span* span) {
            Insert(Begin(), span);
        }
        
        Span* PopFront() {
            Span* front = _head->_next;
            Erase(front);
            return front;
        }
        
        void Insert(Span* pos, Span* newSpan)
        {
            assert(pos);
            assert(newSpan);
            Span* prev = pos->_prev;//获取前一个节点，newSpan插在pos前面
            prev->_next = newSpan;
            newSpan->_prev = prev;
            newSpan->_next = pos;
            pos->_prev = newSpan;//从前往后更新指针
        }; 
        void Erase(Span* pos)
        {
            //pos就是待删除的节点
            assert(pos);
            assert(pos != _head);
            Span* prev = pos->_prev;//获取前一个节点,用于更新前一个节点的next指针
            Span* next = pos->_next;//获取后一个节点,用于更新后一个节点的prev指针
            prev->_next = next;
            next->_prev = prev;//依旧是从前往后更新指针
        };                   
    
    public:
        std::mutex _mtx;  // 这个锁后面用
    
    private:
        Span* _head;      // 哨兵头节点
    };