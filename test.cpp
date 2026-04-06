#include <iostream>
#include "mysmart_ptr.h"

using namespace std;

// 测试类
class Test {
public:
    Test() { cout << "Test 构造()" << endl; }
    Test(int x, string s) {
        cout << "Test 构造(int, string)：" << x << " " << s << endl;
    }
    ~Test() { cout << "Test 析构" << endl; }
    void show() { cout << "调用 Test::show()" << endl; }
};

// 测试 shared_ptr + make_smart
void test_shared() {
    cout << "===== 测试 shared_ptr =====" << endl;

    // 使用 make_smart 创建智能指针
    auto p1 = make_smart<Test>();
    p1->show();

    // 支持有参构造
    auto p2 = make_smart<Test>(100, "hello");

    // 拷贝共享
    auto p3 = p2;
    cout << "shared_ptr 引用计数：" << p3.use_count() << endl;

    cout << endl;
}

// 测试 unique_ptr + make_smart
void test_unique() {
    cout << "===== 测试 unique_ptr =====" << endl;

    //使用 make_smart 创建 unique_ptr
    auto p1 = make_smart<Test>();
    p1->show();

    // 移动所有权
    auto p2 = std::move(p1);
    cout << "p1 移动后是否为空：" << (p1.get() == nullptr) << endl;

    // reset 释放
    p2.reset();
    cout << endl;
}

int main() {
    test_shared();
    test_unique();

    cout << "===== 全部测试完成 =====" << endl;
    return 0;
}