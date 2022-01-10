#ifndef _RUA_SCHED_DOZER_DEFAULT_HPP
#define _RUA_SCHED_DOZER_DEFAULT_HPP

#include "abstract.hpp"

#include "../../macros.hpp"
#include "../../thread/dozer.hpp"

#include <functional>

namespace rua {

inline dozer_i
make_default_dozer(std::function<dozer_i()> make_dozer = []() -> dozer_i {
	return std::make_shared<thread_dozer>();
}) {
	static std::function<dozer_i()> make = std::move(make_dozer);
	return make();
}

} // namespace rua

#endif
