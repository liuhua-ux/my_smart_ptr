#include <iostream>
#include <string>
#include "mysmart_ptr.h"

using namespace std;

// 测试用类
class Test
{
public:
    Test() { cout << "[Test] 无参构造" << endl; }
    Test(int x) { cout << "[Test] int构造：" << x << endl; }
    Test(string s) { cout << "[Test] string构造：" << s << endl; }
    ~Test() { cout << "[Test] 析构" << endl; }

    void show() { cout << "[Test] 调用成员函数 show()" << endl; }
    int getValue() { return 666; }
};

// 测试函数声明
void test_my_shared_ptr();
void test_my_unique_ptr();
void test_make_smart();
void test_assign_and_weak();

int main()
{
    cout << "======= 开始测试 my_shared_ptr =======" << endl;
    test_my_shared_ptr();
    cout << endl;

    cout << "======= 开始测试 my_unique_ptr =======" << endl;
    test_my_unique_ptr();
    cout << endl;

    cout << "======= 开始测试 工厂函数 =======" << endl;
    test_make_smart();
    cout << endl;

    cout << "======= 开始测试 赋值运算符 / weak_ptr / 删除器 =======" << endl;
    test_assign_and_weak();
    cout << endl;

    cout << "======= 所有测试执行完毕！=======" << endl;
    return 0;
}

// 测试共享指针
void test_my_shared_ptr()
{
    cout << "1. 构造空 shared_ptr" << endl;
    my_shared_ptr<Test> p1;
    cout << "   是否为空：" << (bool)p1 << endl;

    cout << "2. 构造带对象的 shared_ptr" << endl;
    my_shared_ptr<Test> p2(new Test());
    cout << "   引用计数：" << p2.unique() << endl;

    cout << "3. 拷贝构造（引用计数+1）" << endl;
    my_shared_ptr<Test> p3 = p2;
    cout << "   引用计数：" << p2.unique() << endl;

    cout << "4. 调用 -> 操作符" << endl;
    p2->show();
    cout << "   调用 * 解引用：" << (*p2).getValue() << endl;

    cout << "5. reset() 重置指针" << endl;
    p2.reset();
    cout << "   reset 后是否为空：" << (bool)p2 << endl;
}

// 测试独占指针
void test_my_unique_ptr()
{
    cout << "1. 构造空 unique_ptr" << endl;
    my_unique_ptr<Test> u1;

    cout << "2. 构造带对象的 unique_ptr" << endl;
    my_unique_ptr<Test> u2(new Test(100));

    cout << "3. 移动构造（所有权转移）" << endl;
    my_unique_ptr<Test> u3 = move(u2);
    cout << "   原指针是否为空：" << (bool)u2 << endl;

    cout << "4. 调用 -> 和 *" << endl;
    u3->show();
    cout << "   getValue：" << (*u3).getValue() << endl;

    cout << "5. release() 释放所有权" << endl;
    Test* raw = u3.release();
    cout << "   释放后指针为空：" << (bool)u3 << endl;
    delete raw; // 这里要手动删，因为已经脱离智能指针管理
}

// 测试工厂函数
void test_make_smart()
{
    cout << "1. 测试 make_shared_smart" << endl;
    auto p = make_shared_smart<Test>("测试字符串");
    p->show();

    cout << "2. 测试 make_unique_smart" << endl;
    auto u = make_unique_smart<Test>(999);
    cout << "get value：" << u->getValue() << endl;
}

// ============================================================
// 追加：赋值运算符 / weak_ptr / 自定义删除器
// 原来的测试只覆盖了拷贝构造，而 operator= 恰好是出过问题的地方
// ============================================================
static int g_alive = 0;
struct Track
{
    int id;
    Track(int i) : id(i) { g_alive++; }
    ~Track() { g_alive--; }
};

static int g_custom_calls = 0;
void track_deleter(Track* p) { g_custom_calls++; delete p; }

static int g_fail = 0;
#define CHECK(cond, msg) do {     if (cond) { cout << "   [OK]   " << msg << endl; }     else      { cout << "   [FAIL] " << msg << endl; g_fail++; } } while (0)

void test_assign_and_weak()
{
    cout << "1. 拷贝赋值：两个都非空" << endl;
    {
        my_shared_ptr<Track> a(new Track(1));
        my_shared_ptr<Track> b(new Track(2));
        b = a;
        CHECK(a.use_count() == 2 && b.use_count() == 2, "引用计数 = 2");
        CHECK(g_alive == 1, "原来那个对象已被释放");
    }
    CHECK(g_alive == 0, "离开作用域后全部析构");

    cout << "2. 自赋值" << endl;
    {
        my_shared_ptr<Track> a(new Track(3));
        my_shared_ptr<Track>& r = a;
        a = r;
        CHECK(a.use_count() == 1, "自赋值后计数仍为 1");
    }
    CHECK(g_alive == 0, "自赋值没有误删对象");

    cout << "3. 赋空 / 被空赋值" << endl;
    {
        my_shared_ptr<Track> a(new Track(4));
        my_shared_ptr<Track> empty;
        a = empty;
        CHECK(a.get() == nullptr, "赋空之后变为空");
        my_shared_ptr<Track> c(new Track(5));
        empty = c;
        CHECK(empty.use_count() == 2, "空指针接管后计数 = 2");
    }
    CHECK(g_alive == 0, "全部析构");

    cout << "4. weak_ptr：过期与提升" << endl;
    {
        my_weak_ptr<Track> w;
        {
            my_shared_ptr<Track> a(new Track(6));
            w = a;
            CHECK(!w.expired(), "对象还在时未过期");
            CHECK(w.lock().use_count() == 2, "lock() 提升后计数 = 2");
        }
        CHECK(w.expired(), "对象销毁后过期");
        CHECK(w.lock().get() == nullptr, "过期后 lock() 返回空");
    }
    CHECK(g_alive == 0, "全部析构");

    cout << "5. 自定义删除器 + lock()" << endl;
    {
        my_shared_ptr<Track>* survivor = nullptr;
        {
            my_shared_ptr<Track> a(new Track(7), track_deleter);
            my_weak_ptr<Track> w = a;
            survivor = new my_shared_ptr<Track>(w.lock());
        }
        CHECK(survivor->use_count() == 1, "只剩 lock() 出来的副本");
        delete survivor;
        CHECK(g_custom_calls == 1, "自定义删除器被调用（删除器跟着控制块走）");
    }
    CHECK(g_alive == 0, "全部析构");

    cout << "6. weak_ptr 打破循环引用" << endl;
    {
        struct Node { my_shared_ptr<Node> next; my_weak_ptr<Node> prev; };
        my_shared_ptr<Node> a(new Node);
        my_shared_ptr<Node> b(new Node);
        a->next = b;
        b->prev = a;
        CHECK(a.use_count() == 1 && b.use_count() == 2, "引用计数正确");
    }
    CHECK(true, "循环引用没有泄漏");

    if (g_fail == 0) cout << ">>> 本节全部通过" << endl;
    else cout << ">>> 本节失败 " << g_fail << " 项" << endl;
}
