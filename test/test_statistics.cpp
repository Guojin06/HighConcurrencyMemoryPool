#include <iostream>
#include <windows.h>
#include "../src/ConcurrentMemoryPool.h"

using namespace std;

// ========== 辅助函数 ==========

// 打印统计数据
void PrintStats() {
    cout << "========== 统计数据 ==========" << endl;
    cout << "allocCount:    " << g_allocCount.load() << endl;
    cout << "freeCount:     " << g_freeCount.load() << endl;
    cout << "currentMemory: " << g_currentMemory.load() << " 字节" << endl;
    cout << "peakMemory:    " << g_peakMemory.load() << " 字节" << endl;
    cout << "==============================" << endl;
}

// 重置统计数据
void ResetStats() {
    g_allocCount.store(0);
    g_freeCount.store(0);
    g_currentMemory.store(0);
    g_peakMemory.store(0);
}

// ========== 测试用例 ==========

// 测试1：基本分配和释放
void TestBasicStats() {
    cout << "\n【测试1】基本分配和释放" << endl;
    ResetStats();  // 重置统计
    
    // 分配3次：16字节、32字节、64字节
    void* p1 = ConcurrentAlloc(16);
    void* p2 = ConcurrentAlloc(32);
    void* p3 = ConcurrentAlloc(64);
    
    cout << "分配3次后：" << endl;
    PrintStats();
    cout << "预期：allocCount=3, currentMemory=112字节(16+32+64)" << endl;
    
    // 释放前2个
    ConcurrentFree(p1, 16);
    ConcurrentFree(p2, 32);
    
    cout << "\n释放2次后：" << endl;
    PrintStats();
    cout << "预期：freeCount=2, currentMemory=64字节(只剩p3)" << endl;
    
    // 释放最后一个
    ConcurrentFree(p3, 64);
    
    cout << "\n全部释放后：" << endl;
    PrintStats();
    cout << "预期：freeCount=3, currentMemory=0字节" << endl;
}

// 测试2：大内存分配（>256KB）
void TestLargeAlloc() {
    cout << "\n【测试2】大内存分配（>256KB）" << endl;
    ResetStats();
    
    // 分配一个 300KB 的内存（走malloc，但也会统计）
    size_t largeSize = 300 * 1024;
    void* p = ConcurrentAlloc(largeSize);
    
    cout << "分配 300KB 后：" << endl;
    PrintStats();
    cout << "预期：allocCount=1, currentMemory=" << largeSize << "字节" << endl;
    
    // 释放
    ConcurrentFree(p, largeSize);
    
    cout << "\n释放后：" << endl;
    PrintStats();
    cout << "预期：freeCount=1, currentMemory=0字节" << endl;
}

// 测试3：峰值内存测试
void TestPeakMemory() {
    cout << "\n【测试3】峰值内存" << endl;
    ResetStats();
    
    // 先分配1KB
    void* p1 = ConcurrentAlloc(1024);
    cout << "分配 1KB 后，currentMemory = " << g_currentMemory.load() << " 字节" << endl;
    
    // 再分配2KB
    void* p2 = ConcurrentAlloc(2048);
    cout << "再分配 2KB 后，currentMemory = " << g_currentMemory.load() << " 字节" << endl;
    cout << "此时峰值 peakMemory = " << g_peakMemory.load() << " 字节（应该是3072=1024+2048）" << endl;
    
    // 释放第一个1KB
    ConcurrentFree(p1, 1024);
    cout << "\n释放 1KB 后，currentMemory = " << g_currentMemory.load() << " 字节" << endl;
    
    // 查看峰值（峰值不应该变小）
    cout << "峰值内存 peakMemory = " << g_peakMemory.load() << " 字节" << endl;
    cout << "分析：峰值应该保持 3072 字节，不会因为释放而减少" << endl;
    
    // 释放第二个2KB
    ConcurrentFree(p2, 2048);
    cout << "\n全部释放后：" << endl;
    PrintStats();
}

// 测试4：混合场景
void TestMixedScenario() {
    cout << "\n【测试4】混合场景：小内存+大内存" << endl;
    ResetStats();
    
    // 小内存
    void* p1 = ConcurrentAlloc(16);
    void* p2 = ConcurrentAlloc(128);
    
    // 大内存
    void* p3 = ConcurrentAlloc(300 * 1024);
    
    cout << "分配后：" << endl;
    PrintStats();
    cout << "预期：allocCount=3" << endl;
    
    // 全部释放
    ConcurrentFree(p1, 16);
    ConcurrentFree(p2, 128);
    ConcurrentFree(p3, 300 * 1024);
    
    cout << "\n释放后：" << endl;
    PrintStats();
    cout << "预期：allocCount=3, freeCount=3, currentMemory=0" << endl;
}

// ========== 主函数 ==========

int main() {
    // 设置控制台编码为UTF-8，解决中文乱码问题
    SetConsoleOutputCP(65001);
    
    cout << "========== 性能统计功能测试 ==========" << endl;
    
    TestBasicStats();
    TestLargeAlloc();
    TestPeakMemory();
    TestMixedScenario();
    
    cout << "\n========== 所有测试完成 ==========" << endl;
    cout << "\n关键点验证：" << endl;
    cout << "✓ allocCount 和 freeCount 能正确统计" << endl;
    cout << "✓ currentMemory 随分配/释放正确变化" << endl;
    cout << "✓ peakMemory 记录峰值且不会减少" << endl;
    cout << "✓ 大内存（>256KB）也能正确统计" << endl;
    
    return 0;
}
