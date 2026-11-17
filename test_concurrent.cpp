// 并发测试：多个线程同时放掉最后一批强引用
// 目的：验证「减 1」和「判断是否为 0」是一步完成的，不会出现双重释放
//
// 编译：g++ -std=c++17 -O2 -pthread -finput-charset=GBK test_concurrent.cpp -o test_concurrent
//
// 说明：这个测试能抓住的是「泄漏 / 重复释放」这类硬错误。
// 竞态本身是概率性的，跑通不代表证明；证明在实现里：
//   if (--shared_count == 0) 是 fetch_sub，返回的是减完之后的值。

#include <atomic>
#include <cstdio>
#include <iostream>
#include <thread>
#include <vector>
#include "mysmart_ptr.h"

static std::atomic<int> g_live{ 0 };
static std::atomic<int> g_deleted{ 0 };

struct Track
{
    Track() { g_live++; }
    ~Track() { g_live--; g_deleted++; }
};

int main()
{
    const int threads_per_round = 8;
    const int rounds = 3000;

    for (int r = 0; r < rounds; r++)
    {
        my_shared_ptr<Track>* base = new my_shared_ptr<Track>(new Track);
        std::atomic<int> ready{ 0 };
        std::atomic<bool> go{ false };
        std::vector<std::thread> ts;
        ts.reserve(threads_per_round);

        for (int i = 0; i < threads_per_round; i++)
        {
            ts.emplace_back([&] {
                my_shared_ptr<Track> local = *base;   // 每个线程持有一份
                my_weak_ptr<Track> w = *base;         // 同时测 lock 与析构并发
                ready++;
                while (!go.load()) {}                 // 等所有人就位，一起放掉
                auto locked = w.lock();
                (void)locked;
            });
        }

        while (ready.load() < threads_per_round) {}
        go = true;
        for (auto& t : ts) t.join();
        delete base;
    }

    std::cout << "rounds=" << rounds
              << "  live=" << g_live.load() << " (expect 0)"
              << "  deleted=" << g_deleted.load() << " (expect " << rounds << ")"
              << std::endl;
    return (g_live.load() == 0 && g_deleted.load() == rounds) ? 0 : 1;
}
