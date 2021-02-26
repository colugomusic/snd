#pragma once

#include <cstdint>

namespace snd {

using ChannelCount = std::uint16_t;
using FrameCount = std::uint32_t;
using SampleRate = std::uint32_t;
using BitDepth = std::uint16_t;

template <class T>
struct Range
{
	T begin;
	T end;
};

template <class T>
struct XY
{
	T x;
	T y;
};

template <class T>
struct Rect
{
	XY<T> position;
	XY<T> size;
};

}