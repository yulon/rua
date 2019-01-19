#ifndef _RUA_RASSERT_HPP
#define _RUA_RASSERT_HPP

#include "console.hpp"
#include "str.hpp"

#include <cstdlib>

#define RUA_RASSERT(_exp) { \
	if (!(_exp)) { \
		rua::cerr( \
			"Assertion failed!", \
			rua::eol, \
			"File:", std::string(__FILE__) + ":" + std::to_string(__LINE__), \
			rua::eol, \
			"Expression:", #_exp \
		); \
		abort(); \
	} \
}

#endif
