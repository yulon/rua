#ifndef _RUA_SCHED_UTIL_HPP
#define _RUA_SCHED_UTIL_HPP

#include "scheduler.hpp"

#include "../macros.hpp"
#include "../thread.hpp"

#include <atomic>

namespace rua {

inline thread_loc<scheduler_i> &_scheduler_storage() {
	static thread_loc<scheduler_i> s;
	return s;
}

inline scheduler_i _get_scheduler(thread_loc<scheduler_i> &ss) {
	auto p = ss.value();
	if (!p) {
		return thread_scheduler::instance();
	}
	return p;
}

inline scheduler_i get_scheduler() {
	return _get_scheduler(_scheduler_storage());
}

class scheduler_guard {
public:
	scheduler_guard(scheduler_i s) {
		auto &ss = _scheduler_storage();
		_previous = std::move(ss.value());
		ss.emplace(std::move(s));
	}

	~scheduler_guard() {
		_scheduler_storage().emplace(std::move(_previous));
	}

	scheduler_i previous() {
		if (!_previous) {
			return thread_scheduler::instance();
		}
		return _previous;
	}

private:
	scheduler_i _previous;
};

RUA_FORCE_INLINE void yield() {
	get_scheduler()->yield();
}

RUA_FORCE_INLINE void sleep(ms timeout) {
	get_scheduler()->sleep(timeout);
}

class secondary_signaler : public scheduler::signaler {
public:
	constexpr secondary_signaler() : _pri(nullptr), _state(false) {}

	explicit secondary_signaler(scheduler::signaler_i main_sch) :
		_pri(main_sch),
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
