#pragma once

#include "2-pole.h"

namespace snd {
namespace audio {
namespace filter {

template <class T>
class Filter_2Pole_Mono
{
	int sr_;
	float freq_;
	float res_;

	Filter_2Pole<T> filter_;

public:

	Filter_2Pole_Mono();

	T lp() const { return filter_.lp(); }
	T bp() const { return filter_.bp(); }
	T hp() const { return filter_.hp(); }

	void operator()(const T& in);

	void set_freq(float freq, bool recalculate = true);
	void set_res(float res, bool recalculate = true);
	void set_sr(int sr, bool recalculate = true);
	void recalculate();
};

template <class T>
Filter_2Pole_Mono<T>::Filter_2Pole_Mono()
	: sr_(44100)
	, freq_(1.f)
	, res_(0.f)
{
}

template <class T>
void Filter_2Pole_Mono<T>::operator()(const T& in)
{
	filter_(in);
}

template <class T>
void Filter_2Pole_Mono<T>::set_freq(float freq, bool recalc)
{
	freq_ = std::max(0.01f, freq);

	if (recalc)	recalculate();
}

template <class T>
void Filter_2Pole_Mono<T>::set_res(float res, bool recalc)
{
	res_ = res;

	if (recalc)	recalculate();
}

template <class T>
void Filter_2Pole_Mono<T>::set_sr(int sr, bool recalc)
{
	sr_ = sr;

	if (recalc)	recalculate();
}

template <class T>
void Filter_2Pole_Mono<T>::recalculate()
{
	float w_div_2;
	float d;
	float e;

	Filter_2Pole<T>::calculate(sr_, freq_, res_, &w_div_2, &d, &e);

	filter_.set(w_div_2, d, e);
}

}}}