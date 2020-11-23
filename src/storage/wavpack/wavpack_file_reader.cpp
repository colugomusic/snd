#include "wavpack_file_reader.h"
#include <wavpack.h>

namespace snd {
namespace storage {
namespace wavpack {

FileReader::FileReader(const std::string& utf8_path)
	: utf8_path_(utf8_path)
{
}

WavpackContext* FileReader::open()
{
	int flags = 0;

	flags |= OPEN_2CH_MAX;
	flags |= OPEN_NORMALIZE;

#ifdef _WIN32
	flags |= OPEN_FILE_UTF8;
#endif

	char error[80];

	return WavpackOpenFileInput(utf8_path_.c_str(), error, flags, 0);
}

}}}