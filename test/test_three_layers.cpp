#include "../src/ThreadCache.h"
#include <iostream>
using std::cout;
using std::endl;

// 简单测试：验证三层架构是否能正常工作
int main() {
    cout << "=== 三层架构联调测试 ===" << endl << endl;
    
    // 测试1：小对象分配（走ThreadCache -> CentralCache -> PageCache）
    cout << "测试1：分配小对象（32字节）" << endl;
    void* ptr1 = GetTLSThreadCache()->Allocate(32);
    void* ptr2 = GetTLSThreadCache()->Allocate(32);
    void* ptr3 = GetTLSThreadCache()->Allocate(32);
    
    if (ptr1 && ptr2 && ptr3) {
        cout << "小对象分配成功" << endl;
        cout << "  ptr1 = " << ptr1 << endl;
        cout << "  ptr2 = " << ptr2 << endl;
        cout << "  ptr3 = " << ptr3 << endl;
    } else {
        cout << "小对象分配失败！" << endl;
        return 1;
    }
    
    // 测试2：大对象分配（需要多页）
    cout << "\n测试2：分配大对象（128KB）" << endl;
    void* bigPtr = GetTLSThreadCache()->Allocate(128 * 1024);
    
    if (bigPtr) {
        cout << "大对象分配成功" << endl;
        cout << "  bigPtr = " << bigPtr << endl;
    } else {
        cout << "大对象分配失败！" << endl;
        return 1;
    }
    
    // 测试3：释放对象
    cout << "\n测试3：释放对象" << endl;
    GetTLSThreadCache()->Deallocate(ptr1, 32);
    GetTLSThreadCache()->Deallocate(ptr2, 32);
    GetTLSThreadCache()->Deallocate(ptr3, 32);
    GetTLSThreadCache()->Deallocate(bigPtr, 128 * 1024);
    cout << "对象释放完成" << endl;
    
    // 测试4：再次分配（验证是否能复用）
    cout << "\n测试4：再次分配（验证缓存复用）" << endl;
    void* ptr4 = GetTLSThreadCache()->Allocate(32);
    
    if (ptr4) {
        cout << "再次分配成功" << endl;
        cout << "  ptr4 = " << ptr4 << endl;
        if (ptr4 == ptr3 || ptr4 == ptr2 || ptr4 == ptr1) {
            cout << "  成功复用了之前释放的内存！" << endl;
        }
    }
    
    cout << "\n=== 所有测试通过！ ===" << endl;
    return 0;
}

