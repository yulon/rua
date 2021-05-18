#ifndef _RUA_SCHED_SUSPENDER_DEFAULT_HPP
#define _RUA_SCHED_SUSPENDER_DEFAULT_HPP

#include "abstract.hpp"

#include "../../macros.hpp"
#include "../../thread/suspender.hpp"

#include <functional>

namespace rua {

inline suspender_i make_default_suspender(
	std::function<suspender_i()> make_suspender = []() -> suspender_i {
		return std::make_shared<thread_suspender>();
	}) {
	static std::function<suspender_i()> make = std::move(make_suspender);
	return make();
}

} // namespace rua

#endif
