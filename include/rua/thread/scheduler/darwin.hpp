#ifndef _RUA_THREAD_SCHEDULER_DARWIN_HPP
#define _RUA_THREAD_SCHEDULER_DARWIN_HPP

#include "../../macros.hpp"
#include "../../sched/scheduler/abstract.hpp"
#include "../../types/util.hpp"

#include <dispatch/dispatch.h>
#include <time.h>
#include <unistd.h>

#include <memory>

namespace rua { namespace darwin {

class thread_waker : public waker {
public:
	using native_handle_t = dispatch_semaphore_t;

	thread_waker() : _sem(dispatch_semaphore_create(0)) {}

	virtual ~thread_waker() {
		if (!_sem) {
			return;
		}
		dispatch_release(_sem);
		_sem = nullptr;
	}

	native_handle_t native_handle() {
		return _sem;
	}

	virtual void wake() {
		dispatch_semaphore_signal(_sem);
	}

	void reset() {
		while (!dispatch_semaphore_wait(_sem, DISPATCH_TIME_NOW))
			;
	}

private:
	dispatch_semaphore_t _sem;
};

class thread_scheduler : public scheduler {
public:
	constexpr thread_scheduler(duration yield_dur = 0) :
		_yield_dur(yield_dur), _wkr() {}

	virtual ~thread_scheduler() = default;

	virtual void yield() {
		if (_yield_dur > 1_us) {
			sleep(_yield_dur);
			return;
		}
		for (auto i = 0; i < 3; ++i) {
			if (!sched_yield()) {
				return;
			}
		}
		::usleep(1);
	}

	virtual bool sleep(duration timeout, bool wakeable = false) {
		if (!wakeable) {
			struct timespec ts {
				timeout.seconds<decltype(ts.tv_sec)>(),
					static_cast<decltype(ts.tv_nsec)>(
						timeout.remaining_nanoseconds())
			};
			::nanosleep(&ts, nullptr);
			return false;
		}
		assert(_wkr);
		return !dispatch_semaphore_wait(
			_wkr->native_handle(),
			timeout.nanoseconds<dispatch_time_t, DISPATCH_TIME_FOREVER>());
	}

	virtual waker_i get_waker() {
		if (_wkr) {
			_wkr->reset();
		} else {
			_wkr = std::make_shared<thread_waker>();
		}
		return _wkr;
	}

private:
	duration _yield_dur;
	std::shared_ptr<thread_waker> _wkr;
};

}} // namespace rua::darwin

#endif
