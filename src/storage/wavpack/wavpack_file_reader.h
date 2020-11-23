#pragma once

#include "wavpack_reader.h"

namespace snd {
namespace storage {
namespace wavpack {

class FileReader : public Reader
{
	std::string utf8_path_;

	WavpackContext* open() override;

public:

	FileReader(const std::string& utf8_path);
};

}}}