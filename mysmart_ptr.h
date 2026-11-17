#pragma once

#include <functional>
#include <utility>
#include <atomic>

template <typename T> class my_shared_ptr;
template <typename T> class my_unique_ptr;
template <typename T> class my_weak_ptr;

template <typename T, typename... Args>
my_shared_ptr<T> make_shared_smart(Args&&... args);

template <typename T, typename... Args>
my_unique_ptr<T> make_unique_smart(Args&&... args);

//控制块结构体
struct ControlBlock 
{
	// shared_count：强引用数，决定「对象」什么时候销毁
	// total_count ：强 + 弱 的总引用数，决定「控制块」什么时候销毁
	// 两者都用前置 --（即 fetch_sub）一步完成「减 1 并判断是否为 0」
	std::atomic<unsigned> shared_count{ 0 };
	std::atomic<unsigned> total_count{ 0 };

	// 删除器归控制块所有：任何持有者（包括 weak_ptr::lock() 造出来的副本）
	// 都能拿到同一个删除器，不会像原来那样退化成默认 delete
	std::function<void(void*)> del;
};

//shared指针
template<typename T>
class my_shared_ptr
{
	template <typename U, typename... Args>
	friend my_shared_ptr<U> make_shared_smart(Args&&... args);
	template <typename U>
	friend class my_weak_ptr;

public:
	//构造与析构函数
	my_shared_ptr();
	explicit my_shared_ptr(T*);
	my_shared_ptr(const my_shared_ptr&);
	my_shared_ptr(T*, std::function<void(T*)>);
	~my_shared_ptr();

	//运算符重载
	my_shared_ptr& operator = (const my_shared_ptr&);
	T& operator*() const;
	T* operator->() const;
	explicit operator bool() const;

	//核心函数
	bool unique() const;
	void reset();
	void reset(T*);
	void reset(T*, std::function<void(T*)>);
	long use_count() const;
	T* get() const;

	//内部数据
private:
	ControlBlock* m_cb = nullptr;
	T* m_pobject = nullptr;
};

//构造函数定义
template <typename T>
my_shared_ptr<T>::my_shared_ptr()
	: m_cb(nullptr), m_pobject(nullptr)
{
}

template <typename T>
my_shared_ptr<T>::my_shared_ptr(T* p)
	: m_cb(nullptr), m_pobject(p)
{
	if (m_pobject) 
	{
		m_cb = new ControlBlock;
		m_cb->shared_count = 1;
		m_cb->total_count = 1;
		m_cb->del = [](void* q) { delete static_cast<T*>(q); };
	}
}

template <typename T>
my_shared_ptr<T>::my_shared_ptr(T* p, std::function<void(T*)> del)
	: m_cb(nullptr), m_pobject(p)
{
	if (m_pobject)
	{
		m_cb = new ControlBlock;
		m_cb->shared_count = 1;
		m_cb->total_count = 1;
		// 把 T* 版本的删除器包成 void* 版本存进控制块
		m_cb->del = [del](void* q) { del(static_cast<T*>(q)); };
	}
}

template <typename T>
my_shared_ptr<T>::my_shared_ptr(const my_shared_ptr& rhs)
	: m_cb(rhs.m_cb), m_pobject(rhs.m_pobject)
{
	if (m_cb)
	{
		m_cb->total_count++;
		m_cb->shared_count++;
	}
}

// 析构函数定义
template <typename T>
my_shared_ptr<T>::~my_shared_ptr()
{
	if (m_cb == nullptr) 
	{
		return;
	}

	// 原子地「减 1 并判断是否是最后一个强引用」
	// 原来写成 shared_count--; if (shared_count == 0) 是两步，
	// 两个线程同时把 1 减到 0 时都会读到 0，两边都去 delete → 双重释放
	if (--m_cb->shared_count == 0)
	{
		m_cb->del(m_pobject);
	}

	// 控制块的生死只看总引用数（强 + 弱）
	if (--m_cb->total_count == 0)
	{
		delete m_cb;
		m_cb = nullptr;
	}
}

//运算符重载定义
template <typename T>
my_shared_ptr<T>& my_shared_ptr<T>::operator = (const my_shared_ptr<T>& other)
{
	// 防止自赋值
	if (this == &other)
	{
		return *this;
	}
	// 先放掉自己原来持有的对象：原子减 1，减到 0 说明自己是最后一个
	if (m_cb != nullptr)
	{
		if (--m_cb->shared_count == 0)
		{
			m_cb->del(m_pobject);
		}
		if (--m_cb->total_count == 0)
		{
			delete m_cb;
		}
	}

	// 再接管 other 的那一份
	m_pobject = other.m_pobject;
	m_cb = other.m_cb;
	if (m_cb) 
	{
		m_cb->total_count++;
		m_cb->shared_count++;
	}

	return *this;
}

template <typename T>
T& my_shared_ptr<T>::operator*() const
{
	return *m_pobject;
}
template <typename T>
T* my_shared_ptr<T>::operator->() const
{
	return m_pobject;
}

template <typename T>
my_shared_ptr<T>::operator bool() const
{
	return m_pobject != nullptr;
}

//核心函数定义
template <typename T>
bool my_shared_ptr<T>::unique() const
{
	if (!m_cb) return false;
	return  m_cb->shared_count == 1;
}

template <typename T>
void my_shared_ptr<T>::reset()
{
	if (m_cb != nullptr)
	{
		if (--m_cb->shared_count == 0)
		{
			m_cb->del(m_pobject);
		}
		if (--m_cb->total_count == 0)
		{
			delete m_cb;
			m_cb = nullptr;
		}
	}
	m_pobject = nullptr;
	m_cb = nullptr;
}

template <typename T>
void my_shared_ptr<T>::reset(T* p)
{
	reset();

	m_pobject = p;

	if (m_pobject != nullptr)
	{
		m_cb = new ControlBlock;
		m_cb->shared_count = 1;
		m_cb->total_count = 1;
		m_cb->del = [](void* q) { delete static_cast<T*>(q); };
	}

}

template <typename T>
void my_shared_ptr<T>::reset(T* p, std::function<void(T*)> del)
{
	reset();

	m_pobject = p;

	if (m_pobject != nullptr)
	{
		m_cb = new ControlBlock;
		m_cb->shared_count = 1;
		m_cb->total_count = 1;
		m_cb->del = [del](void* q) { del(static_cast<T*>(q)); };
	}
}

template <typename T>
T* my_shared_ptr<T>::get() const
{
	return m_pobject;
}

template <typename T>
long my_shared_ptr<T>::use_count() const
{
	if (!m_cb) return 0;
	return m_cb->shared_count.load();
}


//unique指针
template <typename T>
class my_unique_ptr
{
	template <typename U, typename... Args>
	friend my_unique_ptr<U> make_unique_smart(Args&&... args);

public:
	//构造与析构函数
	my_unique_ptr() noexcept;
	explicit my_unique_ptr(T* ptr) noexcept;
	my_unique_ptr(my_unique_ptr&& other) noexcept;
	my_unique_ptr(const my_unique_ptr&) = delete;
	~my_unique_ptr() noexcept;

	//运算符重载
	//unique指针移动赋值
	my_unique_ptr& operator=(my_unique_ptr&& other) noexcept;
	//禁止拷贝赋值
	my_unique_ptr& operator=(const my_unique_ptr&) = delete;
	T& operator*() const noexcept;
	T* operator->() const noexcept;
	explicit operator bool() const noexcept;

	//核心接口
	T* get() const noexcept;
	void reset() noexcept;
	T* release() noexcept;

private:
	T* _ptr = nullptr;
};

//构造函数定义
template <typename T>
my_unique_ptr<T>::my_unique_ptr() noexcept :
	_ptr(nullptr)
{
}

template <typename T>
my_unique_ptr<T>::my_unique_ptr(T* ptr) noexcept :
	_ptr(ptr)
{
}

template <typename T>
my_unique_ptr<T>::my_unique_ptr(my_unique_ptr&& other) noexcept
{
	_ptr = other._ptr;
	other._ptr = nullptr;
}

//析构函数定义
template <typename T>
my_unique_ptr<T>::~my_unique_ptr() noexcept
{
	delete _ptr;
}

//运算符重载
template <typename T>
my_unique_ptr<T>& my_unique_ptr<T>::operator=(my_unique_ptr&& other) noexcept
{
	if (this == &other) return *this;
	delete _ptr;
	_ptr = other._ptr;
	other._ptr = nullptr;

	return *this;
}

template <typename T>
T& my_unique_ptr<T>::operator*() const noexcept
{
	return *_ptr;
}

template <typename T>
T* my_unique_ptr<T>::operator->() const noexcept
{
	return _ptr;
}

template <typename T>
my_unique_ptr<T>::operator bool() const noexcept
{
	return _ptr != nullptr;
}

//核心函数定义
template <typename T>
T* my_unique_ptr<T>::get() const noexcept
{
	return _ptr;
}

template <typename T>
void my_unique_ptr<T>::reset() noexcept
{
	if (_ptr != nullptr)
	{
		delete _ptr;
		_ptr = nullptr;
	}
}

template <typename T>
T* my_unique_ptr<T>::release() noexcept
{
	T* temp = _ptr;
	_ptr = nullptr;
	return temp;
}

// 辅助函数：智能指针构造
template <typename T, typename... Args>
my_shared_ptr<T> make_shared_smart(Args&&... args)
{
	my_unique_ptr<T> guard(new T(std::forward<Args>(args)...));
	my_shared_ptr<T> sp(guard.get());
	guard.release();
	return sp;
}

template <typename T, typename... Args>
my_unique_ptr<T> make_unique_smart(Args&&... args)
{
	return my_unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template<typename T>
class my_weak_ptr
{
public:
	//构造函数
	my_weak_ptr();        
	my_weak_ptr(const my_weak_ptr& other); 
	my_weak_ptr(const my_shared_ptr<T>& other);  

	//析构函数
	~my_weak_ptr();

	//赋值重载
	my_weak_ptr& operator=(const my_weak_ptr& other);
	my_weak_ptr& operator=(const my_shared_ptr<T>& other);

	// 核心功能函数
	void reset();  
	bool expired() const;  
	my_shared_ptr<T> lock() const;  

	// 辅助函数
	long use_count() const;  

private:
	T* m_pobject = nullptr;
	ControlBlock* m_cb = nullptr;

	friend class my_shared_ptr<T>;
};

//构造函数定义
template<typename T>
my_weak_ptr<T>::my_weak_ptr()
	: m_pobject(nullptr), m_cb(nullptr)
{
}

template<typename T>
my_weak_ptr<T>::my_weak_ptr(const my_weak_ptr& other)
	: m_pobject(other.m_pobject), m_cb(other.m_cb)
{
	if (m_cb)
	{
		m_cb->total_count++;
	}
}

template<typename T>
my_weak_ptr<T>::my_weak_ptr(const my_shared_ptr<T>& other)
	: m_pobject(other.m_pobject), m_cb(other.m_cb)
{
	if (m_cb) 
	{
		m_cb->total_count++;
	}
}

// 析构函数
template<typename T>
my_weak_ptr<T>::~my_weak_ptr()
{
	reset();
}

//运算符重载
template<typename T>
my_weak_ptr<T>& my_weak_ptr<T>::operator=(const my_weak_ptr& other)
{
	if (this == &other) return *this;

	reset();

	m_pobject = other.m_pobject;
	m_cb = other.m_cb;

	if (m_cb) 
	{
		m_cb->total_count++;
	}

	return *this;
}

template<typename T>
my_weak_ptr<T>& my_weak_ptr<T>::operator=(const my_shared_ptr<T>& other)
{
	reset();

	m_pobject = other.m_pobject;
	m_cb = other.m_cb;

	if (m_cb) 
	{
		m_cb->total_count++;
	}

	return *this;
}


//核心函数定义
template<typename T>
void my_weak_ptr<T>::reset()
{
	if (m_cb) 
	{
		// 总引用数减到 0 才销毁控制块
		if (--m_cb->total_count == 0)
		{
			delete m_cb;
		}
	}

	m_pobject = nullptr;
	m_cb = nullptr;
}

template<typename T>
bool my_weak_ptr<T>::expired() const
{
	return !m_cb || m_cb->shared_count == 0;
}

template<typename T>
my_shared_ptr<T> my_weak_ptr<T>::lock() const
{
	my_shared_ptr<T> sp;
	if (!m_cb)
		return sp;

	// 不能先 expired() 再 ++：两步之间的对象可能刚被销毁。
	// 用 CAS 把「判断 > 0」和「加 1」合成一步。
	unsigned old = m_cb->shared_count.load();
	do
	{
		if (old == 0)
			return sp;          // 对象已经没了
	} while (!m_cb->shared_count.compare_exchange_weak(old, old + 1));

	m_cb->total_count++;        // 多了一个强引用，总引用数也要加
	sp.m_pobject = m_pobject;
	sp.m_cb = m_cb;
	return sp;
}

template<typename T>
long my_weak_ptr<T>::use_count() const
{
	if (!m_cb)return 0;
	return m_cb->shared_count;
}