#ifndef _rua_thread_dozer_posix_hpp
#define _rua_thread_dozer_posix_hpp

#include "../../time.hpp"
#include "../../util.hpp"

#include <sched.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

#include <cassert>
#include <memory>

namespace rua { namespace posix {

class waker {
public:
	using native_handle_t = sem_t *;

	waker() {
		$need_close = !sem_init(&$sem, 0, 0);
	}

	~waker() {
		if (!$need_close) {
			return;
		}
		sem_destroy(&$sem);
		$need_close = false;
	}

	native_handle_t native_handle() {
		return &$sem;
	}

	void wake() {
		sem_post(&$sem);
	}

	void reset() {
		while (!sem_trywait(&$sem))
			;
	}

private:
	sem_t $sem;
	bool $need_close;
};

class dozer {
public:
	constexpr dozer() : $wkr() {}

	bool doze(duration timeout = duration_max()) {
		assert($wkr);
		assert(timeout >= 0);

		if (timeout == duration_max()) {
			return !sem_wait($wkr->native_handle()) || errno != ETIMEDOUT;
		}
		if (!timeout) {
			return !sem_trywait($wkr->native_handle()) || errno != ETIMEDOUT;
		}
		auto ts = (now().to_unix().elapsed() + timeout).c_timespec();
		return !sem_timedwait($wkr->native_handle(), &ts) || errno != ETIMEDOUT;
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

}} // namespace rua::posix

#endif
