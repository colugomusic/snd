#pragma once

namespace snd {
namespace audio {
namespace filter {

class Filter_2Pole_Allpass
{
public:

	struct BQAP
	{
		float a0, a1, a2, b0, b1, b2;
	};

private:

	BQAP bqap_;
	float ap_;
	float zdfbk_val_0_;
	float zdfbk_val_1_;
	float zdfbk_val_2_;
	float zdfbk_val_3_;

public:

	float ap() const { return ap_; }
	void process_frame(float in);

	void set(const BQAP& bqap);

	static void calculate(float sr, float freq, float res, BQAP* bqap);
};

}}}