#pragma once

#pragma warning(push, 0)
#include <DSP/MLDSPOps.h>
#pragma warning(pop)

namespace snd {
namespace audio {

template <size_t ROWS>
class EnvFollower
{
public:

	EnvFollower()
	{
		reset();
	}

    void set_SR(int SR)
    {
        if (SR == params_.SR) return;

        params_.SR = SR;

        recalculate();
    }

    void set_time(float time)
    {
        params_.time = time;

        recalculate();
    }

    ml::DSPVectorArray<ROWS> operator()(const ml::DSPVectorArray<ROWS>& in)
    {
		ml::DSPVectorArray<ROWS> out;

		for (int r = 0; r < ROWS; r++)
		{
			const auto& in_row = in.constRow(r);
			auto& out_row = out.row(r);

			for (int i = 0; i < kFloatsPerDSPVector; i++)
			{
				const auto x = in_row[i];

				z_[r] = ((std::abs(x) - z_[r]) * w_) + z_[r];

				out_row[i] = z_[r];
			}
		}

		return out;
    }

	static float calculate_w(int SR, float time)
	{
        return 1.0f - (std::pow(2.0f, -1.0f / (std::max(0.02f, time * float(SR)))));
	}

	void reset()
	{
		w_ = 0.0f;
		z_.fill(0.0f);
	}

private:

    void recalculate()
    {
        w_ = calculate_w(params_.SR, params_.time);
    }

    struct
    {
        int SR = 44100;
        float time = 0.0f;
    } params_;

    float w_ = 0.0f;
	std::array<float, ROWS> z_;
};

template <size_t ROWS>
class EnvFollowerAR
{
public:

	EnvFollowerAR()
	{
		reset();
	}

    void set_SR(int SR)
    {
        if (SR == params_.SR) return;

        params_.SR = SR;

        recalculate();
    }

    void set_attack(float attack)
    {
        params_.attack = attack;

        recalculate();
    }

    void set_release(float release)
    {
        params_.release = release;

        recalculate();
    }

    ml::DSPVectorArray<ROWS> operator()(const ml::DSPVectorArray<ROWS>& in)
    {
		ml::DSPVectorArray<ROWS> out;

		for (int r = 0; r < ROWS; r++)
		{
			const auto& in_row = in.constRow(r);
			auto& out_row = out.row(r);

			for (int i = 0; i < kFloatsPerDSPVector; i++)
			{
				const auto x = in_row[i];
				const auto y = std::abs(x) - z_[r];
				const auto w = y >= 0 ? wa_ : wr_;

				z_[r] = (y * w) + z_[r];

				out_row[i] = z_[r];
			}
		}

		return out;
    }

	void reset()
	{
		wa_ = 0.0f;
		wr_ = 0.0f;
		z_.fill(0.0f);
	}

private:

    void recalculate()
    {
        wa_ = EnvFollower<ROWS>::calculate_w(params_.SR, params_.attack);
        wr_ = EnvFollower<ROWS>::calculate_w(params_.SR, params_.release);
    }

    struct
    {
        int SR = 44100;
        float attack = 0.0f;
        float release = 0.0f;
    } params_;

    float wa_ = 0.0f;
    float wr_ = 0.0f;
	std::array<float, ROWS> z_;
};
} //
} // snd