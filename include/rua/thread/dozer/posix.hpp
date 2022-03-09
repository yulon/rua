#ifndef _RUA_THREAD_DOZER_POSIX_HPP
#define _RUA_THREAD_DOZER_POSIX_HPP

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
		_need_close = !sem_init(&_sem, 0, 0);
	}

	~waker() {
		if (!_need_close) {
			return;
		}
		sem_destroy(&_sem);
		_need_close = false;
	}

	native_handle_t native_handle() {
		return &_sem;
	}

	void wake() {
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

class dozer {
public:
	constexpr dozer() : _wkr() {}

	bool doze(duration timeout = duration_max()) {
		assert(_wkr);
		assert(timeout >= 0);

		if (timeout == duration_max()) {
			return !sem_wait(_wkr->native_handle()) || errno != ETIMEDOUT;
		}
		if (!timeout) {
			return !sem_trywait(_wkr->native_handle()) || errno != ETIMEDOUT;
		}
		auto ts = (now().to_unix().elapsed() + timeout).c_timespec();
		return !sem_timedwait(_wkr->native_handle(), &ts) || errno != ETIMEDOUT;
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

}} // namespace rua::posix

#endif
