#ifndef _RUA_THREAD_SCHEDULER_POSIX_HPP
#define _RUA_THREAD_SCHEDULER_POSIX_HPP

#include "../../macros.hpp"
#include "../../sched/scheduler/abstract.hpp"
#include "../../types/util.hpp"

#include <sched.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

#include <memory>

namespace rua { namespace posix {

class thread_resumer : public resumer {
public:
	using native_handle_t = sem_t *;

	thread_resumer() {
		_need_close = !sem_init(&_sem, 0, 0);
	}

	virtual ~thread_resumer() {
		if (!_need_close) {
			return;
		}
		sem_destroy(&_sem);
		_need_close = false;
	}

	native_handle_t native_handle() {
		return &_sem;
	}

	virtual void resume() {
		sem_post(&_sem);
	}

	void reset() {
		while (!sem_trywait(&_sem))
			;
	}

private:
	sem_t _sem;
	bool _need_close;
};

class thread_scheduler : public scheduler {
public:
	constexpr thread_scheduler(duration yield_dur = 0) :
		_yield_dur(yield_dur), _rsmr() {}

	virtual ~thread_scheduler() = default;

	virtual void yield() {
		if (_yield_dur > 1_us) {
			sleep(_yield_dur);
			return;
		}
#ifdef _POSIX_PRIORITY_SCHEDULING
		for (auto i = 0; i < 3; ++i) {
			if (!sched_yield()) {
				return;
			}
		}
#endif
		::usleep(1);
	}

	virtual void sleep(duration timeout) {
		auto ts = timeout.c_timespec();
		::nanosleep(&ts, nullptr);
	}

	virtual bool suspend(duration timeout) {
		assert(_rsmr);

		if (timeout == duration_max()) {
			return !sem_wait(_rsmr->native_handle());
		}
		auto ts = timeout.c_timespec();
		return !sem_timedwait(_rsmr->native_handle(), &ts);
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

}} // namespace rua::posix

#endif
