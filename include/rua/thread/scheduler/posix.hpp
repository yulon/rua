#ifndef _RUA_THREAD_SCHEDULER_POSIX_HPP
#define _RUA_THREAD_SCHEDULER_POSIX_HPP

#include "../../limits.hpp"
#include "../../macros.hpp"
#include "../../sched/scheduler/abstract.hpp"

#include <sched.h>
#include <semaphore.h>
#include <unistd.h>

#include <memory>

namespace rua { namespace posix {

class thread_waker : public waker {
public:
	using native_handle_t = sem_t *;

	thread_waker() {
		_need_close = !sem_init(&_sem, 0, 0);
	}

	virtual ~thread_waker() {
		if (!_need_close) {
			return;
		}
		sem_destroy(&_sem);
		_need_close = false;
	}

	native_handle_t native_handle() {
		return &_sem;
	}

	virtual void wake() {
		sem_post(&_sem);
	}

private:
	sem_t _sem;
	bool _need_close;
};

class thread_scheduler : public scheduler {
public:
	constexpr thread_scheduler(ms yield_dur = 0) :
		_yield_dur(yield_dur), _wkr() {}

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

	virtual bool sleep(ms timeout, bool wakeable = false) {
		if (!wakeable) {
			auto us_c = us(timeout).count();
			::usleep(
				static_cast<int64_t>(nmax<int>()) < us_c
					? nmax<int>()
					: static_cast<int>(us_c));
			return false;
		}
		assert(_wkr);
		if (timeout == duration_max()) {
			return sem_wait(_wkr->native_handle()) != ETIMEDOUT;
		}
		struct timespec ts {
			static_cast<int64_t>(
				nmax<decltype(ts.tv_sec)>()) < timeout.s_count()
				? nmax<decltype(ts.tv_sec)>()
				: static_cast<decltype(ts.tv_sec)>(timeout.s_count()),
				static_cast<decltype(ts.tv_nsec)>(timeout.extra_ns_count())
		};
		return sem_timedwait(_wkr->native_handle(), &ts) != ETIMEDOUT;
	}

	virtual waker_i get_waker() {
		if (!_wkr) {
			_wkr = std::make_shared<thread_waker>();
		}
		return _wkr;
	}

private:
	ms _yield_dur;
	std::shared_ptr<thread_waker> _wkr;
};

}} // namespace rua::posix

#endif
