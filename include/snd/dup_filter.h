#pragma once

#include <functional>

namespace snd {

template <class T>
class DupFilter
{
	bool flag_ = false;
	T prev_ = T();
	std::function<void(T)> callback_;

public:

	DupFilter(std::function<void(T)> callback);
	void operator()(T in);
	void copy(const DupFilter<T>& rhs);
	void reset();
};

template <class T>
DupFilter<T>::DupFilter(std::function<void(T)> callback)
	: callback_(callback)
{
}

template <class T>
void DupFilter<T>::operator()(T in)
{
	if (!flag_)
	{
		callback_(in);
		prev_ = in;
		flag_ = true;
		return;
	}

	if (in != prev_)
	{
		callback_(in);
		prev_ = in;
	}
}

template <class T>
void DupFilter<T>::copy(const DupFilter<T>& rhs)
{
	flag_ = rhs.flag_;
	prev_ = rhs.prev_;
}

template <class T>
void DupFilter<T>::reset()
{
	flag_ = false;
	prev_ = T();
}

}