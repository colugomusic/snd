#pragma once

#pragma warning(push, 0)
#include <DSP/MLDSPOps.h>
#pragma warning(pop)

namespace snd {

template <class T, int ROWS>
struct DSPVectorArrayType {};

template <int ROWS>
struct DSPVectorArrayType<int, ROWS>
{
	using Type = ml::DSPVectorArrayInt<ROWS>;
};

template <int ROWS>
struct DSPVectorArrayType<float, ROWS>
{
	using Type = ml::DSPVectorArray<ROWS>;
};

template <class T, int ROWS = 1>
class DiffDetector
{
	bool init_ = false;
	std::array<T, ROWS> val_;

public:

	void clear()
	{
		init_ = false;
	}

	bool operator()(const typename DSPVectorArrayType<T, ROWS>::Type& in)
	{
		if (!init_)
		{
			init_ = true;

			for (int r = 0; r < ROWS; r++)
			{
				val_[r] = in.constRow(r)[kFloatsPerDSPVector - 1];
			}

			return true;
		}

		for (int r = 0; r < ROWS; r++)
		{
			const auto& row = in.constRow(r);

			for (int i = 0; i < kFloatsPerDSPVector; i++)
			{
				if (row[i] != val_[r])
				{
					val_[r] = row[kFloatsPerDSPVector - 1];

					return true;
				}
			}
		}

		return false;
	}
};

}