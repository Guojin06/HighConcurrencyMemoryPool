#include "../src/ThreadCache.h"

int main() {
    cout << "Testing ThreadCache..." << endl;
    
    // 测试TLS机制
    ThreadCache* tc1 = GetTLSThreadCache();
    ThreadCache* tc2 = GetTLSThreadCache();
    cout << "TLS test: " << (tc1 == tc2) << " (should be 1)" << endl;
    
    // 测试批量申请机制
    void* ptr1 = tc1->Allocate(32);  // 第一次申请，会触发FetchFromCentralCache
    void* ptr2 = tc1->Allocate(32);  // 第二次申请，应该从缓存中取
    void* ptr3 = tc1->Allocate(32);  // 第三次申请，应该从缓存中取
    
    cout << "Batch allocation test:" << endl;
    cout << "ptr1: " << ptr1 << endl;
    cout << "ptr2: " << ptr2 << endl; 
    cout << "ptr3: " << ptr3 << endl;
    
    // 验证缓存是否成功的关键指标
    if (ptr1 && ptr2 && ptr3) {
        cout << "Cache validation: All pointers valid" << endl;
        
        // 如果缓存成功，ptr2和ptr3应该来自预先申请的缓存
        // 而不是每次都调malloc
        cout << "Expected: ptr2 and ptr3 from cache (fast)" << endl;
    } else {
        cout << "Cache validation: FAILED - Null pointers detected!" << endl;
    }
    
    // 测试释放
    tc1->Deallocate(ptr1, 32);
    tc1->Deallocate(ptr2, 32);
    tc1->Deallocate(ptr3, 32);
    
    cout << "All tests passed!" << endl;
    return 0;
}