#include "audio/filter/1-pole.h"
#include "misc.h"

namespace snd {
namespace audio {
namespace filter {
//
//bool Filter_1Pole::needs_recalc(int SR, const ml::DSPVector& freq)
//{
//	if (SR != SR_)
//	{
//		SR_ = SR;
//
//		return true;
//	}
//
//	return diff_freq_(freq);
//}
//
//void Filter_1Pole::operator()(const ml::DSPVector& in)
//{
//	for (int i = 0; i < kFloatsPerDSPVector; i++)
//	{
//		auto a = in[i] - zdfbk_val_;
//		auto b = a * g_[i];
//
//		lp_[i] = b + zdfbk_val_;
//		hp_[i] = in[i] - lp_[i];
//
//		zdfbk_val_ = b + lp_[i];
//	}
//}
//
//void Filter_1Pole::operator()(const ml::DSPVector& in, int SR, const ml::DSPVector& freq)
//{
//	if (needs_recalc(SR, freq)) g_ = calculate_g(SR, freq);
//
//	this->operator()(in);
//}
//
//void Filter_1Pole::set_g(float g)
//{
//	g_ = g;
//}
//
//ml::DSPVector Filter_1Pole::calculate_g(int sr, const ml::DSPVector& freq)
//{
//	auto w_div_2 = blt_prewarp_rat_0_08ct_sr_div_2(sr, freq);
//
//	return w_div_2 / (w_div_2 + 1.f);
//}

}}}