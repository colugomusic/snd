#include "wavpack_memory_reader.h"

namespace snd {
namespace storage {
namespace wavpack {

void MemoryReader::init_stream_reader()
{
	stream_reader_.can_seek = [](void* id) -> int
	{
		return id != nullptr;
	};

	stream_reader_.close = [](void* id) -> int
	{
		return true;
	};

	stream_reader_.get_length = [](void* id) -> std::int64_t
	{
		const auto stream = (Stream*)(id);

		return stream->data_size;
	};

	stream_reader_.get_pos = [](void* id) -> std::int64_t
	{
		const auto stream = (Stream*)(id);

		return stream->pos;
	};

	stream_reader_.push_back_byte = [](void* id, int c) -> int
	{
		const auto stream = (Stream*)(id);

		stream->pos--;

		return c;
	};

	stream_reader_.read_bytes = [](void* id, void* data, std::int32_t bcount) -> std::int32_t
	{
		const auto stream = (Stream*)(id);

		auto read_size = bcount;

		if (std::size_t(stream->pos) + bcount > stream->data_size)
		{
			read_size = std::int32_t(stream->data_size - stream->pos);
		}

		const auto beg = ((const char*)(stream->data)) + stream->pos;
		const auto end = beg + read_size;

		std::copy(beg, end, (char*)(data));

		stream->pos += read_size;

		return read_size;
	};

	stream_reader_.set_pos_abs = [](void* id, std::int64_t pos) -> int
	{
		const auto stream = (Stream*)(id);

		stream->pos = pos;

		return 0;
	};

	stream_reader_.set_pos_rel = [](void* id, std::int64_t delta, int mode) -> int
	{
		const auto stream = (Stream*)(id);

		switch (mode)
		{
			case SEEK_SET:
			{
				stream->pos = delta;
				break;
			}

			case SEEK_CUR:
			{
				stream->pos += delta;
				break;
			}

			case SEEK_END:
			{
				stream->pos = stream->data_size + delta;
				break;
			}
		}

		return 0;
	};

	stream_reader_.truncate_here = nullptr;
	stream_reader_.write_bytes = nullptr;
}

MemoryReader::MemoryReader(const void* data, std::size_t data_size)
{
	init_stream_reader();

	stream_.data = data;
	stream_.data_size = data_size;
}

WavpackContext* MemoryReader::open()
{
	int flags = 0;

	flags |= OPEN_2CH_MAX;
	flags |= OPEN_NORMALIZE;

	char error[80];

	return WavpackOpenFileInputEx64(&stream_reader_, &stream_, nullptr, error, flags, 0);
}

}}}