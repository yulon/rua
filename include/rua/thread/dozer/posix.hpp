#ifndef _RUA_THREAD_DOZER_POSIX_HPP
#define _RUA_THREAD_DOZER_POSIX_HPP

#include "../basic/posix.hpp"

#include "../../macros.hpp"
#include "../../sched/dozer/abstract.hpp"
#include "../../string/conv.hpp"
#include "../../string/join.hpp"
#include "../../types/util.hpp"

#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

#include <atomic>
#include <cassert>
#include <memory>

namespace rua { namespace posix {

class thread_waker : public waker_base {
public:
	thread_waker(pthread_t tid) : _tid(tid) {}

	virtual ~thread_waker() {
		if (!_tid) {
			return;
		}
		_tid = 0;
	}

	virtual void wake() {
		auto state_val = _state.exchange(2);
		if (!state_val || state_val == 2) {
			return;
		}
		while (!pthread_kill(_tid, SIGCONT) && _state.load())
			;
	}

	void reset() {
		_state.store(0);
	}

	std::atomic<uintptr_t> &state() {
		return _state;
	}

private:
	pthread_t _tid;
	std::atomic<uintptr_t> _state;
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
		auto dur = timeout.c_timespec();
		timespec rem;
		while (::nanosleep(&dur, &rem) == -1) {
			dur = rem;
		}
	}

	virtual bool doze(duration timeout = duration_max()) {
		assert(_wkr);

		auto &state = _wkr->state();

		auto state_val = state.load();
		if (state_val == 2) {
			_wkr->reset();
			return true;
		}
		assert(!state_val);
		while (!state.compare_exchange_weak(state_val, 1)) {
			assert(state_val != 1);
			if (state_val == 2) {
				_wkr->reset();
				return true;
			}
		}

		auto dur = timeout.c_timespec();
		timespec rem;
		for (;;) {
			auto is_time_up = ::nanosleep(&dur, &rem) != -1;
			auto state_val = state.load();
			if (state_val == 2) {
				_wkr->reset();
				return true;
			}
			if (timeout == duration_max()) {
				continue;
			}
			if (is_time_up) {
				assert(state_val == 1);
				return state.exchange(0) == 2;
			}
			dur = rem;
		}
		return false;
	}

	virtual waker_i get_waker() {
		if (_wkr) {
			_wkr->reset();
		} else {
			static auto act_r = ([]() -> int {
				static struct sigaction new_act, old_act;
				new_act.sa_handler = [](int sig) {
					if (old_act.sa_handler) {
						old_act.sa_handler(sig);
					}
				};
				new_act.sa_flags = 0;
				sigfillset(&new_act.sa_mask);
				old_act.sa_handler = nullptr;
				return sigaction(SIGCONT, &new_act, &old_act);
			})();
			assert(act_r == 0);
			_wkr = std::make_shared<thread_waker>(this_tid());
		}
		return _wkr;
	}

private:
	duration _yield_dur;
	std::shared_ptr<thread_waker> _wkr;
};

}} // namespace rua::posix

#endif
