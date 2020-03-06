#ifndef _RUA_RASSERT_HPP
#define _RUA_RASSERT_HPP

#include "log.hpp"
#include "string.hpp"

#include <cstdlib>

#define RUA_RASSERT(_exp)                                                      \
	{                                                                          \
		if (!(_exp)) {                                                         \
			rua::err_log(                                                      \
				"Assertion failed!",                                           \
				rua::eol::sys_con,                                             \
				"File:",                                                       \
				std::string(__FILE__) + ":" + std::to_string(__LINE__),        \
				rua::eol::sys_con,                                             \
				"Expression:",                                                 \
				#_exp);                                                        \
			abort();                                                           \
		}                                                                      \
	}

#endif
