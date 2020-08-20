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

}