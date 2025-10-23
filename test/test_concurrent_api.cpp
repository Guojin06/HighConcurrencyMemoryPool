// 测试统一对外接口 - ConcurrentAlloc/ConcurrentFree
#include "../src/ConcurrentMemoryPool.h"
#include <iostream>

using std::cout;
using std::endl;

void TestSmallObject() {
    cout << "=== 测试1: 小对象分配（走内存池） ===" << endl;
    
    // 分配小对象（32字节）
    void* ptr1 = ConcurrentAlloc(32);
    void* ptr2 = ConcurrentAlloc(64);
    void* ptr3 = ConcurrentAlloc(128);
    
    cout << "分配小对象：" << endl;
    cout << "  32B:  " << ptr1 << endl;
    cout << "  64B:  " << ptr2 << endl;
    cout << "  128B: " << ptr3 << endl;
    
    // 释放
    ConcurrentFree(ptr1, 32);
    ConcurrentFree(ptr2, 64);
    ConcurrentFree(ptr3, 128);
    
    cout << "小对象释放完成" << endl << endl;
}

void TestLargeObject() {
    cout << "=== 测试2: 大对象分配（直接malloc） ===" << endl;
    
    // 分配大对象（>256KB）
    void* bigPtr1 = ConcurrentAlloc(512 * 1024);  // 512KB
    void* bigPtr2 = ConcurrentAlloc(1024 * 1024); // 1MB
    
    cout << "分配大对象：" << endl;
    cout << "  512KB: " << bigPtr1 << endl;
    cout << "  1MB:   " << bigPtr2 << endl;
    
    // 释放
    ConcurrentFree(bigPtr1, 512 * 1024);
    ConcurrentFree(bigPtr2, 1024 * 1024);
    
    cout << "大对象释放完成" << endl << endl;
}

void TestBoundary() {
    cout << "=== 测试3: 边界值（256KB） ===" << endl;
    
    // 刚好256KB（走内存池）
    void* ptr1 = ConcurrentAlloc(256 * 1024);
    cout << "256KB（内存池）: " << ptr1 << endl;
    
    // 超过256KB（走malloc）
    void* ptr2 = ConcurrentAlloc(256 * 1024 + 1);
    cout << "256KB+1（malloc）: " << ptr2 << endl;
    
    ConcurrentFree(ptr1, 256 * 1024);
    ConcurrentFree(ptr2, 256 * 1024 + 1);
    
    cout << "边界测试完成" << endl << endl;
}

void TestBatchAlloc() {
    cout << "=== 测试4: 批量分配释放 ===" << endl;
    
    const int count = 100;
    void* ptrs[count];
    
    // 批量分配
    for (int i = 0; i < count; ++i) {
        ptrs[i] = ConcurrentAlloc(32);
    }
    cout << "批量分配100个32字节对象完成" << endl;
    
    // 批量释放
    for (int i = 0; i < count; ++i) {
        ConcurrentFree(ptrs[i], 32);
    }
    cout << "批量释放完成" << endl << endl;
}

int main() {
    cout << "======================================" << endl;
    cout << "   统一接口测试（ConcurrentAlloc/Free）" << endl;
    cout << "======================================" << endl;
    cout << endl;
    
    TestSmallObject();
    TestLargeObject();
    TestBoundary();
    TestBatchAlloc();
    
    cout << "======================================" << endl;
    cout << "   所有测试完成！" << endl;
    cout << "======================================" << endl;
    
    return 0;
}

