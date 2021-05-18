#ifndef _RUA_THREAD_SUSPENDER_DARWIN_HPP
#define _RUA_THREAD_SUSPENDER_DARWIN_HPP

#include "../../macros.hpp"
#include "../../sched/suspender/abstract.hpp"
#include "../../types/util.hpp"

#include <dispatch/dispatch.h>
#include <time.h>
#include <unistd.h>

#include <memory>

namespace rua { namespace darwin {

class thread_resumer : public resumer {
public:
	using native_handle_t = dispatch_semaphore_t;

	thread_resumer() : _sem(dispatch_semaphore_create(0)) {}

	virtual ~thread_resumer() {
		if (!_sem) {
			return;
		}
		dispatch_release(_sem);
		_sem = nullptr;
	}

	native_handle_t native_handle() {
		return _sem;
	}

	virtual void resume() {
		dispatch_semaphore_signal(_sem);
	}

	void reset() {
		while (!dispatch_semaphore_wait(_sem, DISPATCH_TIME_NOW))
			;
	}

private:
	dispatch_semaphore_t _sem;
};

class thread_suspender : public suspender {
public:
	constexpr thread_suspender(duration yield_dur = 0) :
		_yield_dur(yield_dur), _rsmr() {}

	virtual ~thread_suspender() = default;

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

	virtual bool suspend(duration timeout) {
		assert(_rsmr);

		return !dispatch_semaphore_wait(
			_rsmr->native_handle(),
			timeout.nanoseconds<dispatch_time_t, DISPATCH_TIME_FOREVER>());
	}

	virtual resumer_i get_resumer() {
		if (_rsmr) {
			_rsmr->reset();
		} else {
			_rsmr = std::make_shared<thread_resumer>();
		}
		return _rsmr;
	}

private:
	duration _yield_dur;
	std::shared_ptr<thread_resumer> _rsmr;
};

}} // namespace rua::darwin

#endif
