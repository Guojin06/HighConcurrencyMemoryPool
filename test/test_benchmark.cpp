#include <iostream>
#include <chrono>
#include <windows.h>
#include "../src/ConcurrentMemoryPool.h"

using namespace std;

// ========== 辅助函数 ==========

// 测试malloc性能（返回耗时ms）
long long BenchmarkMalloc(size_t size, size_t rounds, const char* testName) {
    cout << "\n【" << testName << "】malloc测试" << endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < rounds; i++) {
        void* p = malloc(size);
        free(p);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    cout << "malloc耗时: " << duration << " ms" << endl;
    return duration;
}

// 测试内存池性能（返回耗时ms）
long long BenchmarkMemoryPool(size_t size, size_t rounds, const char* testName) {
    cout << "\n【" << testName << "】内存池测试" << endl;
    
    // 重置统计数据
    g_allocCount.store(0);
    g_freeCount.store(0);
    g_currentMemory.store(0);
    g_peakMemory.store(0);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < rounds; i++) {
        void* p = ConcurrentAlloc(size);
        ConcurrentFree(p, size);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    cout << "内存池耗时: " << duration << " ms" << endl;
    cout << "分配次数: " << g_allocCount.load() << endl;
    cout << "释放次数: " << g_freeCount.load() << endl;
    cout << "峰值内存: " << g_peakMemory.load() / 1024 << " KB" << endl;
    
    return duration;
}

// 对比测试
void ComparePerformance(size_t size, size_t rounds, const char* testName) {
    cout << "\n========================================" << endl;
    cout << "测试场景: " << testName << endl;
    cout << "对象大小: " << size << " 字节" << endl;
    cout << "测试轮数: " << rounds << " 次" << endl;
    cout << "========================================" << endl;
    
    // 测试malloc
    long long mallocTime = BenchmarkMalloc(size, rounds, testName);
    
    // 测试内存池
    long long mempoolTime = BenchmarkMemoryPool(size, rounds, testName);
    
    // 计算性能提升
    cout << "\n【性能对比】" << endl;
    if (mempoolTime > 0) {
        double speedup = (double)mallocTime / mempoolTime;
        cout << "性能提升: " << speedup << "x" << endl;
        if (speedup > 1.0) {
            cout << "内存池更快 " << (speedup - 1.0) * 100 << "%" << endl;
        } else {
            cout << "malloc更快 " << (1.0 / speedup - 1.0) * 100 << "%" << endl;
        }
    }
}

// ========== 测试用例 ==========

void Test1_SmallObject() {
    // 小对象高频分配
    ComparePerformance(16, 1000000, "小对象高频分配（16字节 x 100万次）");
}

void Test2_MediumObject() {
    // 中等对象
    ComparePerformance(1024, 100000, "中等对象（1KB x 10万次）");
}

void Test3_LargeObject() {
    // 大对象（走malloc）
    ComparePerformance(300 * 1024, 10000, "大对象（300KB x 1万次）");
}

int main() {
    SetConsoleOutputCP(65001);  // 中文显示
    
    cout << "========== 高并发内存池性能测试 ==========" << endl;
    
    Test1_SmallObject();
    Test2_MediumObject();
    Test3_LargeObject();
    
    cout << "\n========== 所有测试完成 ==========" << endl;
    return 0;
}
