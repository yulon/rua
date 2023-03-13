#ifndef _rua_thread_dozer_darwin_hpp
#define _rua_thread_dozer_darwin_hpp

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
		if (semaphore_create(mach_task_self(), &$sem, SYNC_POLICY_FIFO, 0) !=
			KERN_SUCCESS) {
			$sem = 0;
		}
	}

	~waker() {
		if (!$sem) {
			return;
		}
		semaphore_destroy(mach_task_self(), $sem);
		$sem = 0;
	}

	native_handle_t native_handle() {
		return $sem;
	}

	void wake() {
		semaphore_signal($sem);
	}

	void reset() {
		while (semaphore_timedwait($sem, {0, 0}) == KERN_SUCCESS)
			;
	}

private:
	semaphore_t $sem;
};

class dozer {
public:
	constexpr dozer() : $wkr() {}

	bool doze(duration timeout = duration_max()) {
		assert($wkr);
		assert(timeout >= 0);

		if (timeout == duration_max()) {
			return semaphore_wait($wkr->native_handle()) !=
				   KERN_OPERATION_TIMED_OUT;
		}
		return semaphore_timedwait(
				   $wkr->native_handle(),
				   {timeout.seconds<decltype(mach_timespec_t::tv_sec)>(),
					timeout.remaining_nanoseconds<
						decltype(mach_timespec_t::tv_nsec)>()}) !=
			   KERN_OPERATION_TIMED_OUT;
	}

	std::weak_ptr<waker> get_waker() {
		if ($wkr && $wkr.use_count() == 1) {
			$wkr->reset();
			return $wkr;
		}
		return assign($wkr, std::make_shared<waker>());
	}

private:
	std::shared_ptr<waker> $wkr;
};

}} // namespace rua::darwin

#endif
