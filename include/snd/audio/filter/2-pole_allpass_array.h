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

	float operator()(float in);

	void set(const Filter_2Pole_Allpass::BQAP& bqap);
};


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