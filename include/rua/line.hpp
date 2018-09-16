#include "macros.hpp"

namespace rua {
	namespace unix {
		static constexpr const char *line_end = "\n";
	}

	namespace win32 {
		static constexpr const char *line_end = "\r\n";
	}

	namespace darwin {
		static constexpr const char *line_end = "\r";
	}

	#ifdef _WIN32
		static constexpr const char *line_end = win32::line_end;
	#elif RUA_DARWIN
		static constexpr const char *line_end = darwin::line_end;
	#else
		static constexpr const char *line_end = unix::line_end;
	#endif
}
