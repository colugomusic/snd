#pragma once

#include "2-pole_allpass.h"

namespace snd {
namespace audio {
namespace filter {

template <int ROWS, int Size>
class Filter_2Pole_AllpassArray
{
	Filter_2Pole_Allpass<ROWS> filters_[Size];

	int SR_ = 0;
	DiffDetector<float, ROWS> diff_freq_;
	DiffDetector<float, ROWS> diff_res_;

	typename Filter_2Pole_Allpass<ROWS>::BQAP data_;

	bool needs_recalc(int SR, const ml::DSPVectorArray<ROWS>& freq, const ml::DSPVectorArray<ROWS>& res);

public:

	Filter_2Pole_AllpassArray(const typename Filter_2Pole_Allpass<ROWS>::BQAP* data = nullptr);
	ml::DSPVectorArray<ROWS> operator()(const ml::DSPVectorArray<ROWS>& in);
	ml::DSPVectorArray<ROWS> operator()(const ml::DSPVectorArray<ROWS>& in, int SR, const ml::DSPVectorArray<ROWS>& freq, const ml::DSPVectorArray<ROWS>& res);

	void set(const typename Filter_2Pole_Allpass<ROWS>::BQAP& bqap);
	void set_external_data(const typename Filter_2Pole_Allpass<ROWS>::BQAP* data);
};

template <int ROWS, int Size>
Filter_2Pole_AllpassArray<ROWS, Size>::Filter_2Pole_AllpassArray(const typename Filter_2Pole_Allpass<ROWS>::BQAP* data)
{
	set_external_data(&data_);
}

template <int ROWS, int Size>
void Filter_2Pole_AllpassArray<ROWS, Size>::set_external_data(const typename Filter_2Pole_Allpass<ROWS>::BQAP* data)
{
	for (auto& filter : filters_)
	{
		filter.set_external_data(data);
	}
}

template <int ROWS, int Size>
bool Filter_2Pole_AllpassArray<ROWS, Size>::needs_recalc(int SR, const ml::DSPVectorArray<ROWS>& freq, const ml::DSPVectorArray<ROWS>& res)
{
	if (SR != SR_)
	{
		SR_ = SR;

		return true;
	}

	return diff_freq_(freq) || diff_res_(res);
}

template <int ROWS, int Size>
ml::DSPVectorArray<ROWS> Filter_2Pole_AllpassArray<ROWS, Size>::operator()(const ml::DSPVectorArray<ROWS>& in)
{	
	ml::DSPVectorArray<ROWS> out = in;

	for (auto& filter : filters_)
	{
		out = filter(out);
	}

	return out;
}

template <int ROWS, int Size>
ml::DSPVectorArray<ROWS> Filter_2Pole_AllpassArray<ROWS, Size>::operator()(const ml::DSPVectorArray<ROWS>& in, int SR, const ml::DSPVectorArray<ROWS>& freq, const ml::DSPVectorArray<ROWS>& res)
{
	if (needs_recalc(SR, freq, res)) Filter_2Pole_Allpass<ROWS>::calculate(SR, freq, res, &data_);

	return this->operator()(in);
}

template <int ROWS, int Size>
void Filter_2Pole_AllpassArray<ROWS, Size>::set(const typename Filter_2Pole_Allpass<ROWS>::BQAP& bqap)
{
	for (auto& filter : filters_)
	{
		filter.set(bqap);
	}
}

}}}