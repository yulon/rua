#ifndef _RUA_PANIC_HPP
#define _RUA_PANIC_HPP

#include "console.hpp"
#include "str.hpp"

#include <cstdlib>

#define RUA_PANIC(_exp) { \
	if (!(_exp)) { \
		rua::cerr( \
			"Panic!", \
			rua::eol, \
			"File:", std::string(__FILE__) + ":" + std::to_string(__LINE__), \
			rua::eol, \
			"Expression:", #_exp \
		); \
		exit(EXIT_FAILURE); \
	} \
}

#endif
