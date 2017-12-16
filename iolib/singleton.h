#pragma once

template<typename T>
class singleton
{
public:
	~singleton() = default;
	template<typename...Args>
	static T* get_instance(Args&&... args);
	static T* get();
	singleton(const singleton&) = delete;
	singleton& operator=(const singleton&) = delete;
private:
	singleton() = default;
	static T* instance_;
};

template<typename T>
template<typename ...Args>
inline T * singleton<T>::get_instance(Args && ...args)
{
	if (instance_ == nullptr)
		instance_ = new T(std::forward<Args>(args)...);
	return instance_;
}

template<typename T>
inline T * singleton<T>::get()
{
	return instance_;
}

template<typename T>
T* singleton<T>::instance_ = nullptr;
