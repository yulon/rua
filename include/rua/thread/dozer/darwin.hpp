#ifndef _RUA_THREAD_DOZER_DARWIN_HPP
#define _RUA_THREAD_DOZER_DARWIN_HPP

#include "../../time/duration.hpp"
#include "../../util.hpp"

#include <mach/mach.h>
#include <mach/semaphore.h>
#include <time.h>

#include <cassert>
#include <memory>

namespace rua { namespace darwin {

class thread_waker {
public:
	using native_handle_t = semaphore_t;

	thread_waker() {
		if (semaphore_create(mach_task_self(), &_sem, SYNC_POLICY_FIFO, 0) !=
			KERN_SUCCESS) {
			_sem = 0;
		}
	}

	~thread_waker() {
		if (!_sem) {
			return;
		}
		semaphore_destroy(mach_task_self(), _sem);
		_sem = 0;
	}

	native_handle_t native_handle() {
		return _sem;
	}

	void wake() {
		semaphore_signal(_sem);
	}

	void reset() {
		while (semaphore_timedwait(_sem, {0, 0}) == KERN_SUCCESS)
			;
	}

private:
	semaphore_t _sem;
};

class thread_dozer {
public:
	constexpr thread_dozer() : _wkr() {}

	void sleep(duration timeout) {
		auto dur = timeout.c_timespec();
		timespec rem;
		while (::nanosleep(&dur, &rem) == -1) {
			dur = rem;
		}
	}

	bool doze(duration timeout = duration_max()) {
		assert(_wkr);

		if (timeout == duration_max()) {
			return semaphore_wait(_wkr->native_handle()) == KERN_SUCCESS;
		}
		return semaphore_timedwait(
				   _wkr->native_handle(),
				   {timeout.seconds<decltype(mach_timespec_t::tv_sec)>(),
					timeout.remaining_nanoseconds<
						decltype(mach_timespec_t::tv_nsec)>()}) == KERN_SUCCESS;
	}

	std::weak_ptr<thread_waker> get_waker() {
		if (_wkr) {
			_wkr->reset();
		} else {
			_wkr = std::make_shared<thread_waker>();
		}
		return _wkr;
	}

private:
	std::shared_ptr<thread_waker> _wkr;
};

}} // namespace rua::darwin

#endif
