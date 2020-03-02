#ifndef _RUA_LOG_HPP
#define _RUA_LOG_HPP

#include "macros.hpp"
#include "printer.hpp"
#include "stdio.hpp"

namespace rua {

template <typename... Args>
RUA_FORCE_INLINE void log(Args &&... args) {
	printer(sout()).println(std::forward<Args>(args)...);
}

template <typename... Args>
RUA_FORCE_INLINE void err_log(Args &&... args) {
	printer(serr()).println(std::forward<Args>(args)...);
}

} // namespace rua

#endif
