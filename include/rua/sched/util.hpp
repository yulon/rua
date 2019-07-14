#ifndef _RUA_SCHED_UTIL_HPP
#define _RUA_SCHED_UTIL_HPP

#include "abstract.hpp"
#include "../thread.hpp"
#include "../tls.hpp"

namespace rua {

inline tls &_scheduler_storage() {
	static tls s;
	return s;
}

inline scheduler_i _get_scheduler(tls &ss) {
	auto p = ss.get().to<scheduler_i *>();
	if (!p) {
		return thread::scheduler::instance();
	}
	return *p;
}

inline scheduler_i get_scheduler() {
	return _get_scheduler(_scheduler_storage());
}

class scheduler_guard {
public:
	scheduler_guard(scheduler_i s) {
		auto &ss = _scheduler_storage();
		auto p = ss.get().to<scheduler_i *>();
		if (p) {
			_previous = *p;
			*p = std::move(s);
			return;
		}
		p = new scheduler_i(std::move(s));
		ss.set(p);
	}

	~scheduler_guard() {
		auto &ss = _scheduler_storage();

		auto p = ss.get().to<scheduler_i *>();
		assert(p);

		if (!_previous) {
			delete p;
			ss.set(nullptr);
			return;
		}
		*p = std::move(_previous);
	}

	scheduler_i previous() {
		if (!_previous) {
			return thread::scheduler::instance();
		}
		return _previous;
	}

private:
	scheduler_i _previous;
};

inline void yield() {
	get_scheduler()->yield();
}

inline void sleep(size_t timeout) {
	get_scheduler()->sleep(timeout);
}

template <typename Lockable>
class lock_guard {
public:
	lock_guard() : _owner(nullptr) {}

	lock_guard(Lockable &lck) : _owner(&lck) {
		get_scheduler()->lock(lck);
	}

	~lock_guard() {
		unlock();
	}

	void unlock() {
		if (_owner) {
			_owner->unlock();
			_owner = nullptr;
		}
	}

private:
	Lockable *_owner;
};

}

#endif
