#ifndef _RUA_THREAD_THREAD_SCHEDULER_DARWIN_HPP
#define _RUA_THREAD_THREAD_SCHEDULER_DARWIN_HPP

#include "../../limits.hpp"
#include "../../sched/scheduler.hpp"

#include <dispatch/dispatch.h>
#include <unistd.h>

#include <memory>

namespace rua { namespace darwin {

class thread_scheduler : public rua::scheduler {
public:
	static thread_scheduler &instance() {
		static thread_scheduler ds;
		return ds;
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
		using native_handle_t = dispatch_semaphore_t;

		signaler() : _sem(dispatch_semaphore_create(0)) {}

		virtual ~signaler() {
			if (!_sem) {
				return;
			}
			dispatch_release(_sem);
			_sem = nullptr;
		}

		native_handle_t native_handle() {
			return _sem;
		}

		virtual void signal() {
			dispatch_semaphore_signal(_sem);
		}

	private:
		dispatch_semaphore_t _sem;
	};

	virtual signaler_i make_signaler() {
		return std::make_shared<signaler>();
	}

	virtual bool wait(signaler_i sig, ms timeout = duration_max()) {
		assert(sig.type_is<signaler>());

		return !dispatch_semaphore_wait(
			sig.as<signaler>()->native_handle(),
			timeout == duration_max()
				? DISPATCH_TIME_FOREVER
				: static_cast<dispatch_time_t>(timeout.extra_nanoseconds()));
	}

private:
	ms _yield_dur;
};

}} // namespace rua::darwin

#endif
