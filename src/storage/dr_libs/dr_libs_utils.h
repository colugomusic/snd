#pragma once

#include <string>
#include <dr_flac.h>
#include <dr_mp3.h>
#include <dr_wav.h>

namespace snd {
namespace storage {
namespace dr_libs {

namespace flac {

extern drflac* open_file(const std::string& utf8_path);

}

namespace mp3 {

extern bool init_file(drmp3* mp3, const std::string& utf8_path);

}

namespace wav {

extern bool init_file(drwav* wav, const std::string& utf8_path);
extern bool init_file_write(drwav* wav, const std::string& utf8_path, const drwav_data_format* format);
}

}}}