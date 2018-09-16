#include "macros.hpp"

namespace rua {
	static constexpr const char *lf = "\n";

	static constexpr const char *crlf = "\r\n";

	static constexpr const char *cr = "\r";

	#ifdef _WIN32
		static constexpr const char *line_end = crlf;
	#elif RUA_DARWIN
		static constexpr const char *line_end = cr;
	#else
		static constexpr const char *line_end = lf;
	#endif
}
