#pragma once

namespace snd {
namespace audio {
namespace filter {

class Filter_2Pole
{
	float w_div_2_ = 0.0f;
	float d_ = 0.0f;
	float e_ = 0.0f;
	float h_ = 0.0f;
	float zdfbk_val_1_ = 0.0f;
	float zdfbk_val_2_ = 0.0f;
	float lp_ = 0.0f;
	float bp_ = 0.0f;
	float hp_ = 0.0f;

public:

	float lp() const { return lp_; }
	float bp() const { return lp_; }
	float hp() const { return hp_; }
	void process_frame(float in);
	void set(float w_div_2, float d, float e);

	static void calculate(int sr, float freq, float res, float* w_div_2, float* d, float* e);
};

}}}