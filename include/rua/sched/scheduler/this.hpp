#ifndef _RUA_SCHED_SCHEDULER_THIS_HPP
#define _RUA_SCHED_SCHEDULER_THIS_HPP

#include "abstract.hpp"
#include "default.hpp"

#include "../../macros.hpp"
#include "../../thread/var.hpp"

#include <cassert>

namespace rua {

inline scheduler_i &_this_scheduler_ref() {
	static thread_var<scheduler_i> sto;
	if (!sto.has_value()) {
		return sto.emplace(make_default_scheduler());
	}
	assert(sto.value().get());
	return sto.value();
}

inline scheduler_i this_scheduler() {
	return _this_scheduler_ref();
}

class scheduler_guard {
public:
	scheduler_guard(scheduler_i sch) {
		auto &sch_ref = _this_scheduler_ref();
		_prev = std::move(sch_ref);
		sch_ref = std::move(sch);
	}

	scheduler_guard(const scheduler_guard &) = delete;

	scheduler_guard &operator=(const scheduler_guard &) = delete;

	~scheduler_guard() {
		_this_scheduler_ref() = std::move(_prev);
	}

	scheduler_i previous() {
		return _prev;
	}

private:
	scheduler_i _prev;
};

inline void yield() {
	this_scheduler()->yield();
}

inline void sleep(duration timeout) {
	this_scheduler()->sleep(timeout);
}

} // namespace rua

#endif
