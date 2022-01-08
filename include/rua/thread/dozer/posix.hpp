#ifndef _RUA_THREAD_DOZER_POSIX_HPP
#define _RUA_THREAD_DOZER_POSIX_HPP

#include "../../macros.hpp"
#include "../../sched/dozer/abstract.hpp"
#include "../../string/conv.hpp"
#include "../../string/join.hpp"
#include "../../types/util.hpp"

#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>

#include <cassert>
#include <memory>

namespace rua { namespace posix {

class thread_waker : public waker_base {
public:
	using native_handle_t = sem_t *;

	thread_waker() {
		if (!sem_init(&_sem, 0, 0)) {
			_sem_ptr = &_sem;
			return;
		}
		auto name = join(
			"/rua_posix_spare_thread_waker",
			to_string(getpid()),
			to_string(pthread_self()),
			"__");
		_sem_ptr = sem_open(name.c_str(), O_CREAT, 0644, 0);
		assert(_sem_ptr);
		sem_unlink(name.c_str());
	}

	virtual ~thread_waker() {
		if (!_sem_ptr) {
			return;
		}
		if (_sem_ptr == &_sem) {
			sem_destroy(_sem_ptr);
		} else {
			sem_close(_sem_ptr);
		}
		_sem_ptr = nullptr;
	}

	native_handle_t native_handle() {
		return _sem_ptr;
	}

	operator bool() const {
		return _sem_ptr;
	}

	virtual void wake() {
		sem_post(_sem_ptr);
	}

	void reset() {
		while (!sem_trywait(_sem_ptr))
			;
	}

private:
	sem_t *_sem_ptr;
	sem_t _sem;
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
		auto start = tick();
		auto rem = timeout.c_timespec();
		while (::nanosleep(&rem, nullptr)) {
			auto now = tick();
			auto slept = now - start;
			if (slept >= timeout) {
				return;
			}
			rem = (timeout - slept).c_timespec();
		}
	}

	virtual bool doze(duration timeout = duration_max()) {
		assert(_wkr);

		if (timeout == duration_max()) {
			return !sem_wait(_wkr->native_handle());
		}
		auto ts = (now().to_unix().elapsed() + timeout).c_timespec();
		return !sem_timedwait(_wkr->native_handle(), &ts);
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

}} // namespace rua::posix

#endif
