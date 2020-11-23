#pragma once

#include "wavpack_reader.h"
#include <wavpack.h>

namespace snd {
namespace storage {
namespace wavpack {

class MemoryReader : public Reader
{
	WavpackContext* open() override;

	struct Stream
	{
		const void* data;
		std::size_t data_size;
		std::int64_t pos = 0;
	} stream_;

	WavpackStreamReader64 stream_reader_;

	void init_stream_reader();

public:

	MemoryReader(const void* data, std::size_t data_size);
};

}}}