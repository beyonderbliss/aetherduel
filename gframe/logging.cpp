#include "logging.h"
#include <ctime>
#include "fmt.h"
#include "file_stream.h"
#include "localtime.h"
#if EDOPRO_ANDROID
#include "utils.h"
#endif

namespace ygo {

void ErrorLog(epro::stringview msg) {
#if EDOPRO_ANDROID
	const auto log_path = Utils::GetUserStorageDirectory().empty()
		? epro::path_string(EPRO_TEXT("error.log"))
		: Utils::GetUserFolderPathFor(EPRO_TEXT("error.log"));
	FileStream log{ log_path, FileStream::out | FileStream::app };
#else
	FileStream log{ EPRO_TEXT("error.log"), FileStream::out | FileStream::app };
#endif
	if (!log.good())
		return;
	auto now = std::time(nullptr);
	log << epro::format("[{:%Y-%m-%d %H:%M:%S}] {}", epro::localtime(now), msg) << std::endl;
}

}
