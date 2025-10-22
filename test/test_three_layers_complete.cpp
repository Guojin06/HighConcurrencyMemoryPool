// 三层架构完整测试 - Day6完成后的全流程测试
#include "../src/ThreadCache.h"
#include "../src/CentralCache.h"
#include "../src/PageCache.h"

void TestBasicFlow() {
    cout << "=== 测试1: 基本分配释放流程 ===" << endl;
    
    // 分配小对象
    void* ptr1 = GetTLSThreadCache()->Allocate(32);
    void* ptr2 = GetTLSThreadCache()->Allocate(32);
    void* ptr3 = GetTLSThreadCache()->Allocate(32);
    
    cout << "分配3个32字节对象：" << endl;
    cout << "  ptr1 = " << ptr1 << endl;
    cout << "  ptr2 = " << ptr2 << endl;
    cout << "  ptr3 = " << ptr3 << endl;
    
    // 释放对象
    GetTLSThreadCache()->Deallocate(ptr1, 32);
    GetTLSThreadCache()->Deallocate(ptr2, 32);
    GetTLSThreadCache()->Deallocate(ptr3, 32);
    
    cout << "释放完成" << endl;
    cout << endl;
}

void TestBatchRelease() {
    cout << "=== 测试2: 批量归还触发 ===" << endl;
    
    // 分配大量对象，触发批量归还
    const int count = 600;
    void* ptrs[count];
    
    cout << "分配" << count << "个32字节对象..." << endl;
    for (int i = 0; i < count; ++i) {
        ptrs[i] = GetTLSThreadCache()->Allocate(32);
    }
    
    cout << "释放所有对象（会触发批量归还给CentralCache）..." << endl;
    for (int i = 0; i < count; ++i) {
        GetTLSThreadCache()->Deallocate(ptrs[i], 32);
    }
    
    cout << "批量归还完成" << endl;
    cout << endl;
}

void TestPageMerge() {
    cout << "=== 测试3: 页合并算法 ===" << endl;
    
    // 这个测试需要释放完整的Span才能触发页合并
    // 申请大量小对象，全部释放，观察是否触发合并
    
    const int count = 300;
    void* ptrs[count];
    
    cout << "分配" << count << "个32字节对象..." << endl;
    for (int i = 0; i < count; ++i) {
        ptrs[i] = GetTLSThreadCache()->Allocate(32);
    }
    
    cout << "全部释放（可能触发Span归还和页合并）..." << endl;
    for (int i = 0; i < count; ++i) {
        GetTLSThreadCache()->Deallocate(ptrs[i], 32);
    }
    
    cout << "页合并测试完成" << endl;
    cout << endl;
}

void TestLargeObject() {
    cout << "=== 测试4: 大对象分配 ===" << endl;
    
    // 测试大对象（需要多页）
    void* bigPtr1 = GetTLSThreadCache()->Allocate(64 * 1024);  // 64KB
    void* bigPtr2 = GetTLSThreadCache()->Allocate(128 * 1024); // 128KB
    
    cout << "分配大对象：" << endl;
    cout << "  64KB: " << bigPtr1 << endl;
    cout << "  128KB: " << bigPtr2 << endl;
    
    GetTLSThreadCache()->Deallocate(bigPtr1, 64 * 1024);
    GetTLSThreadCache()->Deallocate(bigPtr2, 128 * 1024);
    
    cout << "大对象释放完成" << endl;
    cout << endl;
}

int main() {
    cout << "======================================" << endl;
    cout << "   三层架构完整测试 (Day6)"  << endl;
    cout << "======================================" << endl;
    cout << endl;
    
    TestBasicFlow();
    TestBatchRelease();
    TestPageMerge();
    TestLargeObject();
    
    cout << "======================================" << endl;
    cout << "   所有测试完成！" << endl;
    cout << "======================================" << endl;
    
    return 0;
}

