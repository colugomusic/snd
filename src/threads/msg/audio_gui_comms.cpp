#include "threads/msg/audio_gui_comms.h"

namespace snd {
namespace threads {
namespace msg {

AudioGuiComms::AudioGuiComms(size_t audio_queue_size, size_t gui_queue_size)
	: Comms(audio_queue_size, gui_queue_size)
{
}

void AudioGuiComms::run_audio_tasks(int max)
{
	run_tasks<CHANNEL_AUDIO>(max);
}

void AudioGuiComms::run_gui_tasks(int max)
{
	run_tasks<CHANNEL_GUI>(max);
}

}}}