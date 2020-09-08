#pragma once

#include <atomic>
#include <map>

/*
 * Audio-safe garbage collection
 * 
 * Assumes there is one GUI thread and one audio thread.
 * 
 * Objects of type T can be passed to dispose() to be deleted at a more
 * convenient time, in a more convenient thread.
 * 
 * The objects must have an entry created beforehand, by calling create_entry().
 * 
 * Calling collect() deletes disposed objects.
 * 
 * create_enty() allocates memory and collect() deallocates memory.
 * 
 * dispose() just sets a flag, or does nothing if an entry for the given object
 * was not created beforehand, or if the object was already collected.
 * 
 * example usage:
 * 
 * ---- in GUI thread only ------
 *	object = new Thing();
 *	thing_collector.create_entry(object);
 * 
 * ---- in GUI or audio thread ----
 *	thing_collector.dispose(object);
 * 
 * ---- in GUI thread only ------
 *	thing_collector.collect();
 */


namespace snd {
namespace threads {

template <class T>
class GarbageDisposal
{
	std::map<T*, std::atomic_bool> dispose_flags_;

public:

	GarbageDisposal() = default;
	GarbageDisposal(const GarbageDisposal& rhs) = default;

	~GarbageDisposal()
	{
		collect();
	}

	T* create_entry(T* object)
	{
		dispose_flags_[object] = false;

		return object;
	}

	void collect()
	{
		for (auto it = dispose_flags_.begin(); it != dispose_flags_.end();)
		{
			if (it->second)
			{
				delete it->first;

				it = dispose_flags_.erase(it);
			}
			else
			{
				it++;
			}
		}
	}

	// Calling dispose() on an object more than once is allowed. It will
	// just be ignored.
	void dispose(T* object)
	{
		auto find = dispose_flags_.find(object);

		if (find != dispose_flags_.end())
		{
			find->second = true;
		}
	}
};

}}