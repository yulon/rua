#ifndef _RUA_THREAD_DOZER_DARWIN_HPP
#define _RUA_THREAD_DOZER_DARWIN_HPP

#include "../../macros.hpp"
#include "../../sched/dozer/abstract.hpp"
#include "../../types/util.hpp"

#include <dispatch/dispatch.h>
#include <time.h>
#include <unistd.h>

#include <memory>

namespace rua { namespace darwin {

class thread_waker : public waker_base {
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

class thread_dozer : public dozer_base {
public:
	constexpr thread_dozer(duration yield_dur = 0) :
		_yield_dur(yield_dur), _wkr() {}

	virtual ~thread_dozer() = default;

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

	virtual void sleep(duration timeout) {
		auto ts = timeout.c_timespec();
		::nanosleep(&ts, nullptr);
	}

	virtual bool doze(duration timeout) {
		assert(_wkr);

		return !dispatch_semaphore_wait(
			_wkr->native_handle(),
			timeout.nanoseconds<dispatch_time_t, DISPATCH_TIME_FOREVER>());
	}

	virtual waker get_waker() {
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
