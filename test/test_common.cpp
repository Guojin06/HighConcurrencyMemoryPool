#include "../src/Common.h"

int main() {
    cout << "Testing NextObj..." << endl;
    
    // 测试NextObj函数，看能不能把内存块串成链表
    void* obj1 = malloc(8);
    void* obj2 = malloc(8);
    NextObj(obj1) = obj2;  // obj1指向obj2
    assert(NextObj(obj1) == obj2);
    cout << "NextObj OK" << endl;
    
    // 测试FreeList的Push和Pop
    cout << "Testing FreeList..." << endl;
    FreeList list;
    
    void* ptr1 = malloc(8);
    void* ptr2 = malloc(8);
    
    list.Push(ptr1);
    list.Push(ptr2);  // 头插，ptr2应该在前面
    cout << "Size after push: " << list.Size() << endl;
    
    void* p1 = list.Pop();  // 应该是ptr2
    void* p2 = list.Pop();  // 应该是ptr1
    cout << "Pop worked: " << (p1 == ptr2) << ", " << (p2 == ptr1) << endl;
    
    // 测试SizeClass的对齐和索引计算
    cout << "Testing SizeClass..." << endl;
    cout << "RoundUp(7): " << SizeClass::RoundUp(7) << endl;   // 应该是8
    cout << "RoundUp(9): " << SizeClass::RoundUp(9) << endl;   // 应该是16
    cout << "Index(8): " << SizeClass::Index(8) << endl;       // 应该是0
    cout << "Index(16): " << SizeClass::Index(16) << endl;     // 应该是1
    
    // 简单验证一下结果
    assert(SizeClass::RoundUp(7) == 8);
    assert(SizeClass::RoundUp(9) == 16);
    assert(SizeClass::Index(8) == 0);
    assert(SizeClass::Index(16) == 1);
    
    // Test SpanList
    cout << "Testing SpanList..." << endl;
    SpanList spanList;
    cout << "SpanList empty: " << spanList.Empty() << endl;
    
    // Test SystemAlloc
    cout << "Testing SystemAlloc..." << endl;
    void* sysPtr = SystemAlloc(1);  // 申请1页
    cout << "SystemAlloc got ptr: " << (sysPtr != nullptr) << endl;
    SystemFree(sysPtr);
    
    cout << "All tests passed" << endl;
    
    // Clean up
    free(obj1);
    free(obj2);
    free(ptr1);
    free(ptr2);
    
    return 0;
}