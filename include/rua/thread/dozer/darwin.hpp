#ifndef _RUA_THREAD_DOZER_DARWIN_HPP
#define _RUA_THREAD_DOZER_DARWIN_HPP

#include "../../time/duration.hpp"
#include "../../util.hpp"

#include <mach/mach.h>
#include <mach/semaphore.h>

#include <cassert>
#include <memory>

namespace rua { namespace darwin {

class waker {
public:
	using native_handle_t = semaphore_t;

	waker() {
		if (semaphore_create(mach_task_self(), &_sem, SYNC_POLICY_FIFO, 0) !=
			KERN_SUCCESS) {
			_sem = 0;
		}
	}

	~waker() {
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

class dozer {
public:
	constexpr dozer() : _wkr() {}

	bool doze(duration timeout = duration_max()) {
		assert(_wkr);
		assert(timeout >= 0);

		if (timeout == duration_max()) {
			return semaphore_wait(_wkr->native_handle()) !=
				   KERN_OPERATION_TIMED_OUT;
		}
		return semaphore_timedwait(
				   _wkr->native_handle(),
				   {timeout.seconds<decltype(mach_timespec_t::tv_sec)>(),
					timeout.remaining_nanoseconds<
						decltype(mach_timespec_t::tv_nsec)>()}) !=
			   KERN_OPERATION_TIMED_OUT;
	}

	std::weak_ptr<waker> get_waker() {
		if (_wkr && _wkr.use_count() == 1) {
			_wkr->reset();
			return _wkr;
		}
		return assign(_wkr, std::make_shared<waker>());
	}

private:
	std::shared_ptr<waker> _wkr;
};

}} // namespace rua::darwin

#endif
