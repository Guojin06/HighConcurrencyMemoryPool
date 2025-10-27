// #include <iostream>
// #include <mutex>

// int main() {
//     std::cout << "sizeof(std::mutex) = " 
//               << sizeof(std::mutex) << " 字节" << std::endl;
    
//     std::cout << "缓存行大小 = 64 字节" << std::endl;
    
//     // 计算一个缓存行能放几个mutex
//     int count = 64 / sizeof(std::mutex);
//     std::cout << "一个缓存行可以放 " << count << " 个mutex" << std::endl;
    
//     return 0;
// }
#include <iostream>
#include <mutex>

// 方案1：不加alignas
struct PaddedMutex1 {
    std::mutex mtx;
    char padding[56];
};

// 方案2：加alignas
struct alignas(64) PaddedMutex2 {
    std::mutex mtx;
    char padding[56];
};

int main() {
    PaddedMutex1 arr1[3];
    PaddedMutex2 arr2[3];
    
    std::cout << "=== 不加alignas ===" << std::endl;
    std::cout << "sizeof(PaddedMutex1) = " << sizeof(PaddedMutex1) << std::endl;
    std::cout << "&arr1[0] = " << (void*)&arr1[0] << std::endl;
    std::cout << "&arr1[1] = " << (void*)&arr1[1] << std::endl;
    std::cout << "&arr1[2] = " << (void*)&arr1[2] << std::endl;
    
    // 计算地址差
    size_t diff1 = (char*)&arr1[1] - (char*)&arr1[0];
    std::cout << "地址差 = " << diff1 << " 字节" << std::endl;
    
    std::cout << "\n=== 加alignas(64) ===" << std::endl;
    std::cout << "sizeof(PaddedMutex2) = " << sizeof(PaddedMutex2) << std::endl;
    std::cout << "&arr2[0] = " << (void*)&arr2[0] << std::endl;
    std::cout << "&arr2[1] = " << (void*)&arr2[1] << std::endl;
    std::cout << "&arr2[2] = " << (void*)&arr2[2] << std::endl;
    
    size_t diff2 = (char*)&arr2[1] - (char*)&arr2[0];
    std::cout << "地址差 = " << diff2 << " 字节" << std::endl;
    
    // 检查是否64字节对齐
    std::cout << "\n=== 对齐检查 ===" << std::endl;
    std::cout << "&arr2[0] % 64 = " << ((size_t)&arr2[0] % 64) << std::endl;
    std::cout << "&arr2[1] % 64 = " << ((size_t)&arr2[1] % 64) << std::endl;
    std::cout << "&arr2[2] % 64 = " << ((size_t)&arr2[2] % 64) << std::endl;
    
    return 0;
}