#ifndef _RUA_SCHED_UTIL_HPP
#define _RUA_SCHED_UTIL_HPP

#include "scheduler.hpp"

#include "../macros.hpp"
#include "../thread/var.hpp"
#include "../thread/scheduler.hpp"

#include <atomic>

namespace rua {

inline scheduler_i &_this_scheduler(scheduler_i (*make_default)() = nullptr) {
	static thread_var<scheduler_i> sto;
	static scheduler_i (*mkdft)() =
		make_default ? make_default : []() -> scheduler_i {
		return std::make_shared<thread_scheduler>();
	};
	if (!sto.has_value()) {
		return sto.emplace(mkdft());
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

class secondary_signaler : public scheduler::signaler {
public:
	constexpr secondary_signaler() : _pri(nullptr), _state(false) {}

	explicit secondary_signaler(scheduler::signaler_i primary_signaler) :
		_pri(primary_signaler),
		_state(false) {}

	virtual ~secondary_signaler() = default;

	bool state() const {
		return _state.load();
	}

	virtual void signal() {
		_state.store(true);
		if (_pri) {
			_pri->signal();
		}
	}

	void reset() {
		_state.store(false);
	}

	scheduler::signaler_i primary() const {
		return _pri;
	}

private:
	scheduler::signaler_i _pri;
	std::atomic<bool> _state;
};

} // namespace rua

#endif
