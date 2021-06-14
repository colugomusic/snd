#pragma once

#include "audio_object.hpp"
#include "buffer_sync_signal.hpp"

namespace snd {
namespace threads {

template <class T>
class BufferSyncValue
{

public:

	BufferSyncValue(const BufferSyncSignal& signal)
		: signal_(&signal)
	{
	}

	T audio()
	{
		if (signal_->value() > slot_value_)
		{
			buffer_value_ = value_.load();
			slot_value_ = signal_->value();
		}

		return buffer_value_;
	}

	T gui()
	{
		return value_.load();
	}

	void set(T value)
	{
		value_.store(value);
	}

private:

	const BufferSyncSignal* signal_;
	std::uint32_t slot_value_ = 0;
	std::atomic<T> value_;
	T buffer_value_;
};

template <class T>
class BufferSyncObject
{

public:

	BufferSyncObject(const BufferSyncSignal& signal)
		: signal_(&signal)
	{
	}

	T* audio()
	{
		if (signal_->value() > slot_value_)
		{
			object_.audio().get_next_if_pending();
			slot_value_ = signal_->value();
		}

		return object_.audio().get();
	}

	AudioObjectGUI<T>& gui()
	{
		return object_.gui();
	}

	const AudioObjectGUI<T>& gui() const
	{
		return object_.gui();
	}

	bool pending() const
	{
		return object_.audio().pending();
	}

	AudioObjectSetup<T> make_copy()
	{
		return object_.gui().make_copy();
	}

	void commit(AudioObjectSetup<T>& setup)
	{
		object_.gui().commit(setup);
	}

	template <class ...Args>
	void commit_new(Args... args)
	{
		object_.gui().commit_new(args...);
	}

	T& operator*() { return object_; }
	const T& operator*() const { return object_; }
	T* operator->() { return &object_; }
	const T* operator->() const { return &object_; }

private:

	const BufferSyncSignal* signal_ = nullptr;
	std::uint32_t slot_value_ = 0;
	AudioObject<T> object_;
};

template <class T>
class BufferSyncObjectPair
{
public:

	BufferSyncObjectPair(const BufferSyncSignal& signal)
		: signal_(&signal)
	{
	}

	~BufferSyncObjectPair()
	{
		if (current_[0]) object_.dispose(current_[0]);
		if (current_[1]) object_.dispose(current_[1]);
	}

	void update(int idx)
	{
		if (signal_->value() > slot_value_)
		{
			if (object_.audio().pending())
			{
				if (current_[idx] && current_[idx] != current_[flip(idx)])
				{
					object_.dispose(current_[idx]);
				}

				current_[idx] = object_.audio().get_next_unmanaged();
				recent_ = current_[idx];
			}
		}
	}

	T* audio(int idx) const
	{
		if (current_[idx]) return current_[idx];
		if (current_[flip(idx)]) return current_[flip(idx)];

		return nullptr;
	}

	T* recent() { return recent_; }
	const T* recent() const { return recent_; }

	AudioObjectGUI<T>& gui()
	{
		return object_.gui();
	}

	const AudioObjectGUI<T>& gui() const
	{
		return object_.gui();
	}

	bool pending() const
	{
		return object_.audio().pending();
	}

	AudioObjectSetup<T> make_new()
	{
		return object_.gui().make_new();
	}

	AudioObjectSetup<T> make_copy()
	{
		return object_.gui().make_copy();
	}

	void commit(AudioObjectSetup<T>& setup)
	{
		object_.gui().commit(setup);
	}

	template <class ...Args>
	void commit_new(Args... args)
	{
		object_.gui().commit_new(args...);
	}

private:

	static int flip(int x) { return 1 - x; }

	const BufferSyncSignal* signal_ = nullptr;
	std::uint32_t slot_value_ = 0;
	AudioObject<T> object_;
	T* current_[2] = { nullptr, nullptr };
	T* recent_ = nullptr;
};

}}
