#pragma once

#include <functional>

// 辅助函数：智能指针构造
template <typename T, typename... Args>
shared_ptr<T> make_smart(Args&&... args)
{
	return shared_ptr<T>(new T(std::forward<Args>(args)...));
}

template <typename T, typename... Args>
unique_ptr<T> make_smart(Args&&... args)
{
	return unique_ptr<T>(new T(std::forward<Args>(args)...));
}


//shared指针
template<typename T>
class shared_ptr
{
	template <typename U, typename... Args>
	friend shared_ptr<U> make_smart(Args&&... args);

public:
	//构造与析构函数
	shared_ptr();
	explicit shared_ptr(T*);
	shared_ptr(const shared_ptr&);
	shared_ptr(T*, std::function<void(T*)>);
	~shared_ptr();

	//运算符重载
	shared_ptr& operator = (const shared_ptr&);
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
std::function<void(T*)> shared_ptr<T>::default_del = [](T* p) {delete p; p = nullptr; };

//构造函数定义
template <typename T>
shared_ptr<T>::shared_ptr()
	:m_pobject(nullptr), m_p_use_count(nullptr)
{
}

template <typename T>
shared_ptr<T>::shared_ptr(T* p)
	: m_pobject(p), m_p_use_count(new unsigned(1)), m_del(default_del)
{
}

template <typename T>
shared_ptr<T>::shared_ptr(T* p, std::function<void(T*)> del)
	: m_pobject(p), m_p_use_count(new unsigned(1)), m_del(del)
{
}

template <typename T>
shared_ptr<T>::shared_ptr(const shared_ptr& rhs)
	: m_pobject(rhs.m_pobject), m_p_use_count(rhs.m_p_use_count), m_del(rhs.m_del)
{
	if (m_p_use_count)
		(*m_p_use_count)++;
}

// 析构函数定义
template <typename T>
shared_ptr<T>::~shared_ptr()
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
shared_ptr<T>& shared_ptr<T>::operator = (const shared_ptr<T>& other)
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
T& shared_ptr<T>::operator*() const
{
	return *m_pobject;
}
template <typename T>
T* shared_ptr<T>::operator->() const
{
	return m_pobject;
}

template <typename T>
shared_ptr<T>::operator bool() const
{
	return m_pobject != nullptr;
}

template <typename T>
bool shared_ptr<T>::unique() const
{
	if (!m_p_use_count) return false;
	return *m_p_use_count == 1;
}

template <typename T>
void shared_ptr<T>::reset()
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
void shared_ptr<T>::reset(T* p)
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
void shared_ptr<T>::reset(T* p, std::function<void(T*)> del)
{
	reset();

	m_pobject = p;

	if (m_pobject != nullptr)
	{
		m_p_use_count = new unsigned(1));
	}
	else
	{
		m_p_use_count = nullptr;
	}

	m_del = del;
}

template <typename T>
T* shared_ptr<T>::get() const
{
	return m_pobject;
}


//unique指针
template <typename T>
class unique_ptr
{
	template <typename U, typename... Args>
	friend unique_ptr<U> make_smart(Args&&... args);

public:
	//构造与析构函数
	unique_ptr() noexcept;
	explicit unique_ptr(T* ptr) noexcept;
	unique_ptr(unique_ptr&& other) noexcept;
	unique_ptr(const unique_ptr&) = delete;
	~unique_ptr() noexcept;

	//运算符重载
	//unique指针移动赋值
	unique_ptr& operator=(unique_ptr&& other) noexcept;
	//禁止拷贝赋值
	unique_ptr& operator=(const unique_ptr&) = delete;
	T& operator*() const noexcept;
	T* operator->() const noexcept;

	//核心接口
	T* get() const noexcept;
	void reset() noexcept;
	T* release() noexcept;

private:
	T* _ptr = nullptr;
};

template <typename T>
unique_ptr<T>::unique_ptr() noexcept:
	_ptr(nullptr)
{
}

template <typename T>
unique_ptr<T>::unique_ptr(T* ptr) noexcept :
	_ptr(ptr)
{
}

template <typename T>
unique_ptr<T>::unique_ptr(unique_ptr&& other) noexcept
{
	_ptr = other._ptr;
	other._ptr = nullptr;
}

template <typename T>
unique_ptr<T>::~unique_ptr() noexcept
{
	delete _ptr;
}

template <typename T>
unique_ptr<T>& unique_ptr<T>::operator=(unique_ptr&& other) noexcept
{
	if (this == &other) return *this;
	delete _ptr;
	_ptr = other._ptr;
	other._ptr = nullptr;

	return *this;
}

template <typename T>
T& unique_ptr<T>::operator*() const noexcept
{
	return *_ptr;
}

template <typename T>
T* unique_ptr<T>::operator->() const noexcept
{
	return _ptr;
}

template <typename T>
T* unique_ptr<T>::get() const noexcept
{
	return _ptr;
}

template <typename T>
void unique_ptr<T>::reset() noexcept
{
	if (_ptr != nullptr)
	{
		delete _ptr;
		_ptr = nullptr;
	}
}

template <typename T>
T* unique_ptr<T>::release() noexcept
{
	T* temp = _ptr;
	_ptr = nullptr;
	return temp;
}