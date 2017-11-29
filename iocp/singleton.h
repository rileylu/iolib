#pragma once

template<typename T>
class Singleton
{
public:
	~Singleton() = default;
	template<typename...Args>
	static T* get_instance(Args&&... args);
	static T* get();

	Singleton(const Singleton&) = delete;
	Singleton& operator=(const Singleton&) = delete;
private:
	Singleton() = default;
	static T* instance_;
};

template<typename T>
template<typename ...Args>
inline T * Singleton<T>::get_instance(Args && ...args)
{
	if (instance_ == nullptr)
		instance_ = new T(std::forward<Args>(args)...);
	return instance_;
}

template<typename T>
inline T * Singleton<T>::get()
{
	return instance_;
}

template<typename T>
T* Singleton<T>::instance_ = nullptr;
