#pragma once

namespace snd {
namespace audio {
namespace filter {

class Filter_2Pole_Allpass
{
public:

	struct BQAP
	{
		double a0 = 0.0f;
		double a1 = 0.0f;
		double a2 = 0.0f;
		double b2 = 0.0f;
	};

private:

	BQAP bqap_;
	const BQAP* data_;

	double zdfbk_val_0_ = 0.0f;
	double zdfbk_val_1_ = 0.0f;
	double zdfbk_val_2_ = 0.0f;
	double zdfbk_val_3_ = 0.0f;

public:

	Filter_2Pole_Allpass(const BQAP* data = nullptr);

	float operator()(float in);

	void copy(const Filter_2Pole_Allpass& rhs);

	void set(const BQAP& bqap);
	void set_external_data(const BQAP* data);

	static void calculate(int sr, float freq, float res, BQAP* bqap);
};

}}}