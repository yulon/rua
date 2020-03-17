#ifndef _RUA_SCHED_SCHEDULER_ABSTRACT_THIS_HPP
#define _RUA_SCHED_SCHEDULER_ABSTRACT_THIS_HPP

#include "abstract.hpp"
#include "default.hpp"

#include "../../macros.hpp"
#include "../../thread/var.hpp"

namespace rua {

inline scheduler_i &_this_scheduler() {
	static thread_var<scheduler_i> sto;
	if (!sto.has_value()) {
		return sto.emplace(make_default_scheduler());
	}
	return sto.value();
}

RUA_FORCE_INLINE scheduler_i this_scheduler() {
	return _this_scheduler();
}

class scheduler_guard {
public:
	scheduler_guard(scheduler_i s) {
		auto &sch = _this_scheduler();
		_previous = std::move(sch);
		sch = std::move(s);
	}

	~scheduler_guard() {
		_this_scheduler() = _previous;
	}

	scheduler_i previous() {
		return _previous;
	}

private:
	scheduler_i _previous;
};

RUA_FORCE_INLINE void yield() {
	this_scheduler()->yield();
}

RUA_FORCE_INLINE void sleep(ms timeout) {
	this_scheduler()->sleep(timeout);
}

} // namespace rua

#endif
