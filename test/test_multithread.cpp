#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <windows.h>
#include "../src/ConcurrentMemoryPool.h"

using namespace std;

// ========== 多线程测试函数 ==========

// 单个线程执行的分配/释放任务（malloc版本）
void ThreadTask_Malloc(size_t size, size_t rounds, size_t threadId) {
    for (size_t i = 0; i < rounds; i++) {
        void* p = malloc(size);
        // 简单使用一下内存（防止编译器优化）
        if (p) {
            *(char*)p = threadId;
        }
        free(p);
    }
}

// 单个线程执行的分配/释放任务（内存池版本）
void ThreadTask_MemoryPool(size_t size, size_t rounds, size_t threadId) {
    for (size_t i = 0; i < rounds; i++) {
        void* p = ConcurrentAlloc(size);
        if (p) {
            *(char*)p = threadId;
        }
        ConcurrentFree(p, size);
    }
}

// 【新增】批量分配后批量释放（malloc版本）
void ThreadTask_Malloc_Batch(size_t size, size_t rounds, size_t threadId) {
    vector<void*> ptrs;
    ptrs.reserve(rounds);  // 预分配，避免扩容
    
    // 第1阶段：批量分配
    for (size_t i = 0; i < rounds; i++) {
        void* p = malloc(size);
        if (p) {
            *(char*)p = threadId;
            ptrs.push_back(p);
        }
    }
    
    // 第2阶段：批量释放
    for (void* p : ptrs) {
        free(p);
    }
}

// 【新增】批量分配后批量释放（内存池版本）
void ThreadTask_MemoryPool_Batch(size_t size, size_t rounds, size_t threadId) {
    vector<void*> ptrs;
    ptrs.reserve(rounds);  // 预分配，避免扩容
    
    // 第1阶段：批量分配
    for (size_t i = 0; i < rounds; i++) {
        void* p = ConcurrentAlloc(size);
        if (p) {
            *(char*)p = threadId;
            ptrs.push_back(p);
        }
    }
    
    // 第2阶段：批量释放
    for (void* p : ptrs) {
        ConcurrentFree(p, size);
    }
}

// ========== 多线程性能测试框架 ==========

// 多线程malloc测试
long long BenchmarkMalloc_MultiThread(size_t threadCount, size_t size, size_t roundsPerThread) {
    cout << "\n【malloc多线程测试】线程数: " << threadCount << endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 创建多个线程
    vector<thread> threads;
    for (size_t i = 0; i < threadCount; i++) {
        threads.emplace_back(ThreadTask_Malloc, size, roundsPerThread, i);
    }
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    cout << "malloc耗时: " << duration << " ms" << endl;
    cout << "总分配次数: " << threadCount * roundsPerThread << " 次" << endl;
    
    return duration;
}

// 多线程内存池测试
long long BenchmarkMemoryPool_MultiThread(size_t threadCount, size_t size, size_t roundsPerThread) {
    cout << "\n【内存池多线程测试】线程数: " << threadCount << endl;
    
#ifdef ENABLE_STATS
    // 重置统计数据
    g_allocCount.store(0);
    g_freeCount.store(0);
    g_currentMemory.store(0);
    g_peakMemory.store(0);
#endif
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 创建多个线程
    vector<thread> threads;
    for (size_t i = 0; i < threadCount; i++) {
        threads.emplace_back(ThreadTask_MemoryPool, size, roundsPerThread, i);
    }
    
    // 等待所有线程完成
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    cout << "内存池耗时: " << duration << " ms" << endl;
#ifdef ENABLE_STATS
    cout << "分配次数: " << g_allocCount.load() << endl;
    cout << "释放次数: " << g_freeCount.load() << endl;
    cout << "峰值内存: " << g_peakMemory.load() << " 字节 (" 
         << (double)g_peakMemory.load() / 1024 / 1024 << " MB)" << endl;
#endif
    
    return duration;
}

// ========== 性能对比 ==========

void CompareMultiThreadPerformance(size_t threadCount, size_t size, size_t roundsPerThread, const char* testName) {
    cout << "\n========================================" << endl;
    cout << "测试场景: " << testName << endl;
    cout << "对象大小: " << size << " 字节" << endl;
    cout << "线程数: " << threadCount << endl;
    cout << "每线程轮数: " << roundsPerThread << " 次" << endl;
    cout << "总操作次数: " << threadCount * roundsPerThread << " 次" << endl;
    cout << "========================================" << endl;
    
    // 测试malloc
    long long mallocTime = BenchmarkMalloc_MultiThread(threadCount, size, roundsPerThread);
    
    // 测试内存池
    long long mempoolTime = BenchmarkMemoryPool_MultiThread(threadCount, size, roundsPerThread);
    
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
        
        // 计算吞吐量（每秒操作次数）
        double throughput_mempool = (double)(threadCount * roundsPerThread) / mempoolTime * 1000;
        cout << "内存池吞吐量: " << throughput_mempool << " ops/s" << endl;
    }
}

// ========== 测试用例 ==========

void Test_2Threads() {
    CompareMultiThreadPerformance(2, 16, 100000, "2线程并发（16字节 x 每线程10万次）");
}

void Test_4Threads() {
    CompareMultiThreadPerformance(4, 16, 100000, "4线程并发（16字节 x 每线程10万次）");
}

void Test_8Threads() {
    CompareMultiThreadPerformance(8, 16, 50000, "8线程并发（16字节 x 每线程5万次）");
}

void Test_MixedSize() {
    cout << "\n========================================" << endl;
    cout << "混合大小多线程测试（4线程）" << endl;
    cout << "========================================" << endl;
    
    CompareMultiThreadPerformance(4, 16, 50000, "小对象16B");
    CompareMultiThreadPerformance(4, 128, 20000, "中对象128B");
    CompareMultiThreadPerformance(4, 1024, 10000, "大对象1KB");
}

// 【新增】批量测试的对比函数
void CompareMultiThreadPerformance_Batch(size_t threadCount, size_t size, size_t roundsPerThread, const char* testName) {
    cout << "\n========================================" << endl;
    cout << "【批量模式】" << testName << endl;
    cout << "========================================" << endl;
    
    // 测试malloc批量模式
    cout << "\n【malloc批量模式】" << endl;
    auto start1 = std::chrono::high_resolution_clock::now();
    vector<thread> threads1;
    for (size_t i = 0; i < threadCount; i++) {
        threads1.emplace_back(ThreadTask_Malloc_Batch, size, roundsPerThread, i);
    }
    for (auto& t : threads1) {
        t.join();
    }
    auto end1 = std::chrono::high_resolution_clock::now();
    long long mallocTime = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1).count();
    cout << "malloc耗时: " << mallocTime << " ms" << endl;
    
    // 测试内存池批量模式
    cout << "\n【内存池批量模式】" << endl;
#ifdef ENABLE_STATS
    g_allocCount.store(0);
    g_freeCount.store(0);
    g_currentMemory.store(0);
    g_peakMemory.store(0);
#endif
    
    auto start2 = std::chrono::high_resolution_clock::now();
    vector<thread> threads2;
    for (size_t i = 0; i < threadCount; i++) {
        threads2.emplace_back(ThreadTask_MemoryPool_Batch, size, roundsPerThread, i);
    }
    for (auto& t : threads2) {
        t.join();
    }
    auto end2 = std::chrono::high_resolution_clock::now();
    long long mempoolTime = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2).count();
    
    cout << "内存池耗时: " << mempoolTime << " ms" << endl;
#ifdef ENABLE_STATS
    cout << "峰值内存: " << g_peakMemory.load() << " 字节 (" 
         << (double)g_peakMemory.load() / 1024 / 1024 << " MB)" << endl;
#endif
    
    // 计算性能提升
    cout << "\n【性能对比】" << endl;
    if (mempoolTime > 0 && mallocTime > 0) {
        double speedup = (double)mallocTime / mempoolTime;
        cout << "性能提升: " << speedup << "x" << endl;
        if (speedup > 1.0) {
            cout << " 内存池更快 " << (speedup - 1.0) * 100 << "%！" << endl;
        } else {
            cout << "malloc更快 " << (1.0 / speedup - 1.0) * 100 << "%" << endl;
        }
    }
}

// 【新增】批量模式测试
void Test_BatchMode() {
    cout << "\n========================================" << endl;
    cout << "批量分配释放模式测试（真实场景模拟）" << endl;
    cout << "========================================" << endl;
    
    CompareMultiThreadPerformance_Batch(2, 16, 50000, "2线程批量（16字节 x 5万个）");
    CompareMultiThreadPerformance_Batch(4, 16, 50000, "4线程批量（16字节 x 5万个）");
    CompareMultiThreadPerformance_Batch(8, 16, 50000, "8线程批量（16字节 x 5万个）");
}

int main() {
    SetConsoleOutputCP(65001);  // 中文显示
    
    cout << "========== 高并发内存池多线程性能测试 ==========" << endl;
    cout << "硬件并发数: " << thread::hardware_concurrency() << " 核心" << endl;
    
    // 注释掉即分即放模式，只保留真实场景批量模式
    // Test_2Threads();
    // Test_4Threads();
    // Test_8Threads();
    // Test_MixedSize();
    
    // 【真实场景】批量模式测试
    Test_BatchMode();
    
    cout << "\n========== 所有测试完成 ==========" << endl;
    return 0;
}

