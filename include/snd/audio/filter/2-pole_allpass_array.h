#pragma once

#include "2-pole_allpass.h"

namespace snd {
namespace audio {
namespace filter {

template <int Size>
class Filter_2Pole_AllpassArray
{
	Filter_2Pole_Allpass filters_[Size];

public:

	Filter_2Pole_AllpassArray(const Filter_2Pole_Allpass::BQAP* data = nullptr);
	float operator()(float in);

	void copy(const Filter_2Pole_AllpassArray<Size>& rhs);

	void set(const Filter_2Pole_Allpass::BQAP& bqap);
	void set_external_data(const Filter_2Pole_Allpass::BQAP* data);
};

template <int Size>
Filter_2Pole_AllpassArray<Size>::Filter_2Pole_AllpassArray(const Filter_2Pole_Allpass::BQAP* data)
{
	if (data)
	{
		set_external_data(data);
	}
}

template <int Size>
void Filter_2Pole_AllpassArray<Size>::copy(const Filter_2Pole_AllpassArray<Size>& rhs)
{
	for (int i = 0; i < Size; i++)
	{
		filters_[i].copy(rhs.filters_[i]);
	}
}

template <int Size>
void Filter_2Pole_AllpassArray<Size>::set_external_data(const Filter_2Pole_Allpass::BQAP* data)
{
	for (auto& filter : filters_)
	{
		filter.set_external_data(data);
	}
}

template <int Size>
float Filter_2Pole_AllpassArray<Size>::operator()(float in)
{
	for (auto& filter : filters_)
	{
		in = filter(in);
	}

	return in;
}

template <int Size>
void Filter_2Pole_AllpassArray<Size>::set(const Filter_2Pole_Allpass::BQAP& bqap)
{
	for (auto& filter : filters_)
	{
		filter.set(bqap);
	}
}

}}}