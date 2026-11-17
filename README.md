# my_smart_ptr

不依赖标准库，从零手写 C++ 智能指针：`my_shared_ptr` / `my_unique_ptr` / `my_weak_ptr`。
用于理解 `std::shared_ptr` 的引用计数、控制块生命周期与 `weak_ptr` 打破循环引用的原理。

## 特性

- **`my_shared_ptr<T>`**
  - 引用计数控制块（`ControlBlock`），拷贝构造 / 拷贝赋值维护强引用计数
  - 强引用归零时释放对象；强 + 弱总引用归零时才释放控制块
  - 支持自定义删除器 `std::function<void(T*)>`（删除器存放在控制块中）
  - `use_count()` / `unique()` / `reset()` / `get()`
- **`my_unique_ptr<T>`**
  - 独占所有权：移动构造 / 移动赋值，`= delete` 禁止拷贝
  - `release()` / `reset()` / `get()`
- **`my_weak_ptr<T>`**
  - 独立的总引用计数，不延长对象生命周期
  - `expired()` / `lock()`，`lock()` 用 CAS 原子地「判断存活 + 计数加一」
- **工厂函数**：`make_shared_smart<T>(args...)` / `make_unique_smart<T>(args...)`，完美转发

## 文件

| 文件 | 说明 |
| --- | --- |
| `mysmart_ptr.h` | 全部实现（header-only） |
| `test.cpp` | 测试：拷贝构造 / 赋值运算符 / weak_ptr / 循环引用 / 自定义删除器 |
| `test_concurrent.cpp` | 并发析构测试：8 线程同时放掉最后一批引用，检查无泄漏、无重复释放 |
| `my_smart_ptr.sln` | Visual Studio 工程（本地用，未纳入版本库） |

> 源文件为 **GBK** 编码（Visual Studio 默认）。用 g++ 编译需加 `-finput-charset=GBK`，
> 或用 VS 直接打开工程。

## 构建与测试

```bash
# 功能测试
g++ -std=c++17 -Wall -Wextra -finput-charset=GBK test.cpp -o test && ./test

# 并发测试
g++ -std=c++17 -O2 -pthread -finput-charset=GBK test_concurrent.cpp -o test_concurrent
./test_concurrent
```

```bat
:: Visual Studio
打开 my_smart_ptr.sln，直接 F5
```

测试全部打印 `[OK]`，最后输出「本节全部通过」。

## 设计要点

### 1. 控制块存放删除器

删除器**不能**存在 `shared_ptr` 对象里。否则 `weak_ptr::lock()` 造出来的新 `shared_ptr`
拿不到原删除器，只能退化成默认的 `delete`——只要那个副本是最后一个持有者，
自定义删除器就永远不会执行（这个 bug 已在测试 5 中覆盖）。

因此控制块里放 `std::function<void(void*)> del`，所有持有者共享同一份。

### 2. 两个计数，各管一件事

| 计数 | 含义 | 归零时 |
| --- | --- | --- |
| `shared_count` | 强引用数 | 析构**对象** |
| `total_count` | 强 + 弱总引用数 | 析构**控制块** |

### 3. 「减 1」和「判断是否为 0」必须是一步

```cpp
// 错误：两步之间别的线程可以插进来
m_cb->shared_count--;
if (m_cb->shared_count == 0) { /* delete 对象 */ }

// 正确：前置 -- 是 fetch_sub，返回减完后的值
if (--m_cb->shared_count == 0) { /* delete 对象 */ }
```

否则两个线程同时把计数从 1 减到 0 时，会都读到 0，**两边都去 delete 同一个对象**。

### 4. `weak_ptr::lock()` 用 CAS

```cpp
unsigned old = m_cb->shared_count.load();
do {
    if (old == 0) return {};                          // 对象已经没了
} while (!m_cb->shared_count.compare_exchange_weak(old, old + 1));
```

不能写成「先 `expired()` 判断、再 `++`」——两步之间对象可能刚好被销毁。

## 已知限制

- 不是 `std` 的完整替代：没有 allocator、没有 `make_shared` 的单次分配优化、
  不做数组特化（`T[]`）
- 删除器签名是 `std::function`，比标准库的模板参数多一次间接调用
- 仅供学习与面试讲解，未做大规模并发压测
