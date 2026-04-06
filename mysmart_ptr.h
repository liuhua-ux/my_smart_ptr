#pragma once

#include <functional>
#include <utility>

template <typename T> class my_shared_ptr;
template <typename T> class my_unique_ptr;

template <typename T, typename... Args>
my_shared_ptr<T> make_shared_smart(Args&&... args);

template <typename T, typename... Args>
my_unique_ptr<T> make_unique_smart(Args&&... args);

//shared指针
template<typename T>
class my_shared_ptr
{
	template <typename U, typename... Args>
	friend my_shared_ptr<U> make_shared_smart(Args&&... args);

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
	T* get() const;

	//内部数据
private:
	unsigned* m_p_use_count = nullptr;
	T* m_pobject = nullptr;
	std::function<void(T*)> m_del = default_del;

	//默认delete函数
private:
	static std::function<void(T*)> default_del;
};

//默认delete函数的定义
template <typename T>
std::function<void(T*)> my_shared_ptr<T>::default_del = [](T* p) {delete p; p = nullptr; };

//构造函数定义
template <typename T>
my_shared_ptr<T>::my_shared_ptr()
	:m_pobject(nullptr), m_p_use_count(nullptr)
{
}

template <typename T>
my_shared_ptr<T>::my_shared_ptr(T* p)
	: m_pobject(p), m_p_use_count(new unsigned(1)), m_del(default_del)
{
}

template <typename T>
my_shared_ptr<T>::my_shared_ptr(T* p, std::function<void(T*)> del)
	: m_pobject(p), m_p_use_count(new unsigned(1)), m_del(del)
{
}

template <typename T>
my_shared_ptr<T>::my_shared_ptr(const my_shared_ptr& rhs)
	: m_pobject(rhs.m_pobject), m_p_use_count(rhs.m_p_use_count), m_del(rhs.m_del)
{
	if (m_p_use_count)
		(*m_p_use_count)++;
}

// 析构函数定义
template <typename T>
my_shared_ptr<T>::~my_shared_ptr()
{
	if (m_p_use_count != nullptr) {
		(*m_p_use_count)--;
		if (*m_p_use_count == 0) {
			if (m_del && m_pobject) {
				m_del(m_pobject);
			}
			delete m_p_use_count;
		}
	}
	m_pobject = nullptr;
	m_p_use_count = nullptr;
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
	if (m_p_use_count != nullptr)
	{
		(*m_p_use_count)--;
		if (*m_p_use_count == 0)
		{
			m_del(m_pobject);
			delete m_p_use_count;
		}
	}
	m_pobject = other.m_pobject;
	m_p_use_count = other.m_p_use_count;
	m_del = other.m_del;
	if (m_p_use_count)
		(*m_p_use_count)++;

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

template <typename T>
bool my_shared_ptr<T>::unique() const
{
	if (!m_p_use_count) return false;
	return *m_p_use_count == 1;
}

template <typename T>
void my_shared_ptr<T>::reset()
{
	if (m_p_use_count != nullptr)
	{
		(*m_p_use_count)--;
		if (*m_p_use_count == 0)
		{
			m_del(m_pobject);
			delete m_p_use_count;
		}
	}
	m_pobject = nullptr;
	m_p_use_count = nullptr;
	m_del = default_del;
}

template <typename T>
void my_shared_ptr<T>::reset(T* p)
{
	reset();

	m_pobject = p;

	if (m_pobject != nullptr)
	{
		m_p_use_count = new unsigned(1);
	}
	else
	{
		m_p_use_count = nullptr;
	}

}

template <typename T>
void my_shared_ptr<T>::reset(T* p, std::function<void(T*)> del)
{
	reset();

	m_pobject = p;

	if (m_pobject != nullptr)
	{
		m_p_use_count = new unsigned(1);
	}
	else
	{
		m_p_use_count = nullptr;
	}

	m_del = del;
}

template <typename T>
T* my_shared_ptr<T>::get() const
{
	return m_pobject;
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
	return my_shared_ptr<T>(new T(std::forward<Args>(args)...));
}

template <typename T, typename... Args>
my_unique_ptr<T> make_unique_smart(Args&&... args)
{
	return my_unique_ptr<T>(new T(std::forward<Args>(args)...));
}