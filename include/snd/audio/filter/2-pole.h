#pragma once

namespace snd {
namespace audio {
namespace filter {

class Filter_2Pole
{
	float w_div_2_;
	float d_;
	float e_;
	float zdfbk_val_0_;
	float zdfbk_val_1_;
	float zdfbk_val_2_;
	float lp_;
	float bp_;
	float hp_;


public:

	Filter_2Pole();

	float lp() const { return lp_; }
	float bp() const { return lp_; }
	float hp() const { return hp_; }
	void process_frame(float in);
	void set(float w_div_2, float d, float e);

	static void calculate(float sr, float freq, float res, float* w_div_2, float* d, float* e);
};

}}}