#pragma once

#include "buffer_sync_object.hpp"

namespace snd {
namespace threads {

template <class T>
class AudioSync
{
public:

	AudioSync(const BufferSyncSignal& signal)
		: data_(signal)
	{
		data_.gui().commit_new();
	}

	void sync_copy(std::function<void(T*)> mutator)
	{
		auto copy = data_.gui().make_copy();

		mutator(copy.get());

		data_.gui().commit(copy);
	}

	void sync_new(std::function<void(T*)> mutator)
	{
		auto new_data = data_.gui().make_new();

		mutator(new_data.get());

		data_.gui().commit(new_data);
	}
	
	const T& audio()
	{
		return *(data_.audio());
	}

private:

	BufferSyncObject<T> data_;
};

} // threads
} // snd
