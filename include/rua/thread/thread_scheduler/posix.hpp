#ifndef _RUA_THREAD_THREAD_SCHEDULER_POSIX_HPP
#define _RUA_THREAD_THREAD_SCHEDULER_POSIX_HPP

#include "../../limits.hpp"
#include "../../sched/scheduler.hpp"

#include <sched.h>
#include <semaphore.h>
#include <unistd.h>

#include <memory>

namespace rua { namespace posix {

class thread_scheduler : public rua::scheduler {
public:
	static thread_scheduler &instance() {
		static thread_scheduler inst;
		return inst;
	}

	constexpr thread_scheduler(ms yield_dur = 0) : _yield_dur(yield_dur) {}

	virtual ~thread_scheduler() = default;

	virtual void yield() {
		if (_yield_dur > 1_us) {
			auto us_c = us(_yield_dur).count();
			::usleep(
				static_cast<int64_t>(nmax<int>()) < us_c
					? nmax<int>()
					: static_cast<int>(us_c));
		}
		for (auto i = 0; i < 3; ++i) {
			if (!sched_yield()) {
				return;
			}
		}
		::usleep(1);
	}

	virtual void sleep(ms timeout) {
		auto us_c = us(timeout).count();
		::usleep(
			static_cast<int64_t>(nmax<int>()) < us_c ? nmax<int>()
													 : static_cast<int>(us_c));
	}

	class signaler : public rua::scheduler::signaler {
	public:
		using native_handle_t = sem_t *;

		signaler() {
			_need_close = !sem_init(&_sem, 0, 0);
		}

		virtual ~signaler() {
			if (!_need_close) {
				return;
			}
			sem_destroy(&_sem);
			_need_close = false;
		}

		native_handle_t native_handle() {
			return &_sem;
		}

		virtual void signal() {
			sem_post(&_sem);
		}

	private:
		sem_t _sem;
		bool _need_close;
	};

	virtual signaler_i make_signaler() {
		return std::make_shared<signaler>();
	}

	virtual bool wait(signaler_i sig, ms timeout = duration_max()) {
		assert(sig.type_is<signaler>());

		if (timeout == duration_max()) {
			return sem_wait(sig.as<signaler>()->native_handle()) != ETIMEDOUT;
		}

		timespec ts;
		ts.tv_sec =
			static_cast<int64_t>(nmax<decltype(ts.tv_sec)>()) <
					timeout.total_seconds()
				? nmax<decltype(ts.tv_sec)>()
				: static_cast<decltype(ts.tv_sec)>(timeout.total_seconds());
		ts.tv_nsec =
			static_cast<decltype(ts.tv_nsec)>(timeout.extra_nanoseconds());

		return sem_timedwait(sig.as<signaler>()->native_handle(), &ts) !=
			   ETIMEDOUT;
	}

private:
	ms _yield_dur;
};

}} // namespace rua::posix

#endif
