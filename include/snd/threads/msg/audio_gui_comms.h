#pragma once

#include "msg_comms.h"

namespace snd {
namespace threads {
namespace msg {

/*
* Two way communication channel intended for sending messages between audio and GUI
*/
class AudioGuiComms : public Comms<2>
{
	static constexpr auto CHANNEL_AUDIO = 0;
	static constexpr auto CHANNEL_GUI = 1;

public:

	AudioGuiComms(size_t audio_queue_size = 15, size_t gui_queue_size = 15);

	template <bool MAY_ALLOCATE = false>
	bool audio_run(Channel::Task task);

	template <bool MAY_ALLOCATE = false>
	bool gui_run(Channel::Task task);

	void run_audio_tasks(int max = 0);
	void run_gui_tasks(int max = 0);
};

template <bool MAY_ALLOCATE>
bool AudioGuiComms::audio_run(Channel::Task task)
{
	return run<CHANNEL_AUDIO, MAY_ALLOCATE>(task);
}

template <bool MAY_ALLOCATE>
bool AudioGuiComms::gui_run(Channel::Task task)
{
	return run<CHANNEL_GUI, MAY_ALLOCATE>(task);
}

inline AudioGuiComms::AudioGuiComms(size_t audio_queue_size, size_t gui_queue_size)
	: Comms(audio_queue_size, gui_queue_size)
{
}

inline void AudioGuiComms::run_audio_tasks(int max)
{
	run_tasks<CHANNEL_AUDIO>(max);
}

inline void AudioGuiComms::run_gui_tasks(int max)
{
	run_tasks<CHANNEL_GUI>(max);
}

}}}
