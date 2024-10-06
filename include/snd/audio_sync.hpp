#pragma once

#include <cs_lr_guarded.h>
#include <cs_plain_guarded.h>
#include <vector>

namespace lg = libguarded;

namespace snd {

// Wrapper around lr_guarded for basic publishing of audio data.
// Shared pointers to old versions of the data are kept in a list
// to ensure that they are not deleted by the audio thread.
// garbage_collect() should be called periodically to delete old
// versions.
template <typename T>
struct audio_data {
	template <typename UpdateFn>
	auto modify(UpdateFn&& update_fn) -> void {
		modify({writer_mutex_}, std::forward<UpdateFn>(update_fn));
	}
	auto set(T data) -> void {
		auto lock = std::unique_lock(writer_mutex_);
		auto modify_fn = [data = std::move(data)](T&&) mutable { return std::move(data); };
		modify(std::move(lock), modify_fn);
	}
	auto read() const -> std::shared_ptr<const T> {
		return *ptr_.lock_shared().get();
	}
	auto garbage_collect() -> void {
		auto lock = std::unique_lock(writer_mutex_);
		auto is_garbage = [](const std::shared_ptr<const T>& ptr) { return ptr.use_count() == 1; };
		versions_.erase(std::remove_if(versions_.begin(), versions_.end(), is_garbage), versions_.end());
	}
private:
	template <typename UpdateFn>
	auto modify(std::unique_lock<std::mutex>&& lock, UpdateFn&& update_fn) -> void {
		auto copy = std::make_shared<const T>(writer_data_ = update_fn(std::move(writer_data_)));
		ptr_.modify([copy](std::shared_ptr<const T>& ptr) { ptr = copy; });
		versions_.push_back(std::move(copy));
	}
	T writer_data_;
	std::mutex writer_mutex_;
	lg::lr_guarded<std::shared_ptr<const T>> ptr_;
	std::vector<std::shared_ptr<const T>> versions_;
};

template <typename T>
struct audio_sync {
	audio_sync() { publish(); }
	auto gc() -> void                                         { published_model_.garbage_collect(); }
	auto publish() -> void                                    { published_model_.set(working_model_); }
	auto read() const -> T                                    { auto lock = std::lock_guard{mutex_}; return working_model_; }
	auto rt_read() const -> std::shared_ptr<const T>          { return published_model_.read(); }
	template <typename Fn> auto update(Fn fn) -> void         { auto lock = std::lock_guard{mutex_}; working_model_ = fn(std::move(working_model_)); }
	template <typename Fn> auto update_publish(Fn fn) -> void { update(fn); publish(); }
private:
	mutable std::mutex mutex_;
	T working_model_;
	audio_data<T> published_model_;
};

} // snd
