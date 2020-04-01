#ifndef _RUA_FIBER_HPP
#define _RUA_FIBER_HPP

#include "bytes.hpp"
#include "chrono.hpp"
#include "sched/scheduler.hpp"
#include "types/util.hpp"
#include "ucontext.hpp"

#include <array>
#include <cassert>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <queue>

namespace rua {

class fiber_executor {
public:
	fiber_executor(size_t shared_stack_size = 0x100000) :
		_shared_stk_sz(shared_stack_size), _shared_stk_ix(0), _sch(*this) {}

	class context {
	public:
		void stop() {
			_is_stoped = true;
		}

		void reset_lifetime(ms dur = 0) {
			if (!dur) {
				_end_ti.reset();
			}
			auto now = tick();
			if (dur >= time_max() - now) {
				_end_ti = time_max();
				return;
			}
			_end_ti = now + dur;
		}

	private:
		std::function<void()> _task;

		bool _is_stoped;
		time _end_ti;

		ucontext_t _uc;
		int _shared_stk_ix;
		bytes _stk;

		bool _has_yielded;
		std::shared_ptr<secondary_waker> _wkr;

		friend fiber_executor;
	};

	std::shared_ptr<context>
	execute(std::function<void()> func, ms lifetime = 0) {
		auto ctx = std::make_shared<context>();
		ctx->_task = std::move(func);
		ctx->_is_stoped = false;
		ctx->reset_lifetime(lifetime);
		_fcs.emplace(ctx);
		return ctx;
	}

	std::shared_ptr<context> current() const {
		return _cur_fc;
	}

	operator bool() const {
		return _fcs.size() || _slps.size() || _cws.size();
	}

	// Does not block the current context.
	// The current scheduler will not be used.
	// Mostly used for scheduling of frame tasks.
	void step() {
		auto now = tick();

		if (_slps.size()) {
			_check_slps(now);
		}
		if (_cws.size()) {
			_check_cws(now);
		}
		if (_fcs.empty()) {
			return;
		}

		scheduler_guard sg(_sch);
		_switch_to_worker_uc();
	}

	// May block the current context.
	// The current scheduler will be used.
	void run() {
		if (_fcs.empty() && _slps.empty() && _cws.empty()) {
			return;
		}

		scheduler_guard sg(_sch);
		auto orig_sch = sg.previous();
		_orig_wkr = orig_sch->get_waker();

		auto now = tick();

		if (_slps.size()) {
			_check_slps(now);
		}
		if (_cws.size()) {
			_check_cws(now);
		}

		for (;;) {
			_switch_to_worker_uc();

			now = tick();

			if (_slps.size()) {

				if (_cws.size()) {
					time wake_ti;
					if (_cws.begin()->first < _slps.begin()->first) {
						wake_ti = _cws.begin()->first;
					} else {
						wake_ti = _slps.begin()->first;
					}
					if (wake_ti > now) {
						orig_sch->sleep(wake_ti - now, true);
					} else {
						orig_sch->yield();
					}
					now = tick();
					if (now >= wake_ti) {
						_check_slps(now);
					}
					_check_cws(now);
					continue;
				}

				auto wake_ti = _slps.begin()->first;
				if (wake_ti > now) {
					orig_sch->sleep(wake_ti - now);
				} else {
					orig_sch->yield();
				}
				now = tick();
				_check_slps(now);
				continue;

			} else if (_cws.size()) {

				auto wake_ti = _cws.begin()->first;
				if (wake_ti > now) {
					orig_sch->sleep(wake_ti - now, true);
				} else {
					orig_sch->yield();
				}
				now = tick();
				_check_cws(now);

			} else {
				return;
			}
		}
	}

	class scheduler : public rua::scheduler {
	public:
		scheduler() = default;

		scheduler(fiber_executor &fib_dvr) : _fd(&fib_dvr) {}

		virtual ~scheduler() = default;

		template <typename SlpMap>
		void _sleep(SlpMap &&slp_map, ms timeout) {
			time wake_ti;
			if (!timeout) {
				wake_ti.reset();
			} else if (timeout >= time_max() - tick()) {
				wake_ti = time_max();
			} else {
				wake_ti = tick() + timeout;
			}

			_fd->_cur_fc->_has_yielded = true;
			_fd->_cur_fc->_shared_stk_ix = _fd->_shared_stk_ix;
			slp_map.emplace(wake_ti, _fd->_cur_fc);

			_fd->_prev_fc = std::move(_fd->_cur_fc);
			if (_fd->_fcs.size()) {
				if (!_fd->_try_resume_fcs_front()) {
					_fd->_swap_new_worker_uc(&_fd->_prev_fc->_uc);
				}
			} else {
				swap_ucontext(&_fd->_prev_fc->_uc, &_fd->_orig_uc);
			}
			_fd->_clear_prev_fc();
		}

		virtual bool sleep(ms timeout, bool wakeable = false) {
			if (!wakeable) {
				_sleep(_fd->_slps, timeout);
				return false;
			}

			auto &wkr = _fd->_cur_fc->_wkr;
			if (!wkr->state()) {
				_sleep(_fd->_cws, timeout);
			}
			return wkr->state();
		}

		virtual waker_i get_waker() {
			assert(_fd->_cur_fc);

			auto &wkr = _fd->_cur_fc->_wkr;
			if (wkr) {
				wkr.reset();
			} else {
				wkr = std::make_shared<secondary_waker>(_fd->_orig_wkr);
			}
			return wkr;
		}

		fiber_executor &get_executor() {
			return *_fd;
		}

	private:
		fiber_executor *_fd;
	};

	scheduler &get_scheduler() {
		return _sch;
	}

private:
	std::queue<std::shared_ptr<context>> _fcs;
	std::shared_ptr<context> _cur_fc, _prev_fc;

	std::multimap<time, std::shared_ptr<context>> _slps;
	std::multimap<time, std::shared_ptr<context>> _cws;

	void _check_slps(time now) {
		for (auto it = _slps.begin(); it != _slps.end(); it = _slps.erase(it)) {
			if (now < it->first) {
				break;
			}
			_fcs.emplace(std::move(it->second));
		}
	}

	void _check_cws(time now) {
		for (auto it = _cws.begin(); it != _cws.end();) {
			if (it->second->_wkr->state() || now >= it->first) {
				_fcs.emplace(std::move(it->second));
				it = _cws.erase(it);
				continue;
			}
			++it;
		}
	}

	ucontext_t _orig_uc;

	size_t _shared_stk_sz;
	int _shared_stk_ix;
	bytes _shared_stks[2];
	ucontext_t _new_worker_ucs[2];

	bytes &_cur_stk() {
		return _shared_stks[_shared_stk_ix];
	}

	ucontext_t &_cur_new_worker_uc() {
		return _new_worker_ucs[_shared_stk_ix];
	}

	void _clear_prev_fc() {
		if (!_prev_fc) {
			return;
		}

		assert(!_prev_fc->_stk.size());

		auto &stk = _shared_stks[_prev_fc->_shared_stk_ix];
		auto stk_used = stk(_prev_fc->_uc.sp() - stk.data().uintptr());
		auto rmdr = stk_used.size() % 1024;
		_prev_fc->_stk.resize(stk_used.size() + (rmdr ? 1024 - rmdr : 0));
		_prev_fc->_stk = stk_used;

		_prev_fc.reset();
	}

	bool _try_resume_fcs_front(ucontext_t *oucp = nullptr) {
		if (!_fcs.front()->_stk.size()) {
			return false;
		}

		if (oucp != &_orig_uc &&
			_fcs.front()->_shared_stk_ix == _shared_stk_ix) {
			if (!oucp) {
				set_ucontext(&_orig_uc);
				return true;
			}
			swap_ucontext(oucp, &_orig_uc);
			return true;
		}

		_cur_fc = std::move(_fcs.front());
		_fcs.pop();

		_shared_stk_ix = _cur_fc->_shared_stk_ix;
		auto &cur_stk = _cur_stk();
		cur_stk(cur_stk.size() - _cur_fc->_stk.size()).copy_from(_cur_fc->_stk);
		_cur_fc->_stk.resize(0);

		if (!oucp) {
			set_ucontext(&_cur_fc->_uc);
			return true;
		}
		swap_ucontext(oucp, &_cur_fc->_uc);
		return true;
	}

	void _work() {
		_clear_prev_fc();

		while (_fcs.size()) {
			assert(_fcs.front());
			_try_resume_fcs_front();

			_cur_fc = std::move(_fcs.front());
			assert(!_fcs.front());
			_fcs.pop();

			if (_cur_fc->_is_stoped) {
				continue;
			}

			for (;;) {
				_cur_fc->_task();

				if (_cur_fc->_is_stoped) {
					break;
				}

				if (_cur_fc->_end_ti <= tick()) {
					_cur_fc->_is_stoped = false;
					break;
				}

				if (!_cur_fc->_has_yielded) {
					_slps.emplace(time_zero(), std::move(_cur_fc));
					break;
				}
				_cur_fc->_has_yielded = false;
			}
		}

		set_ucontext(&_orig_uc);
	}

	static void _worker(any_word th1s) {
		th1s.as<fiber_executor *>()->_work();
	}

	void _swap_new_worker_uc(ucontext_t *oucp) {
		if (oucp != &_orig_uc) {
			_shared_stk_ix = !_shared_stk_ix;
		}
		auto &cur_stk = _cur_stk();
		if (!cur_stk) {
			cur_stk.reset(_shared_stk_sz);
			get_ucontext(&_cur_new_worker_uc());
			make_ucontext(&_cur_new_worker_uc(), &_worker, this, cur_stk);
		}
		swap_ucontext(oucp, &_cur_new_worker_uc());
	}

	void _switch_to_worker_uc() {
		while (_fcs.size()) {
			if (!_try_resume_fcs_front(&_orig_uc)) {
				_swap_new_worker_uc(&_orig_uc);
			}
			_clear_prev_fc();
		}
	}

	friend scheduler;
	scheduler _sch;

	waker_i _orig_wkr;
};

inline std::shared_ptr<fiber_executor::context>
co(std::function<void()> func, ms lifetime = 0) {
	auto sch = this_scheduler();
	if (sch) {
		auto fib_sch = sch.as<fiber_executor::scheduler>();
		if (fib_sch) {
			return fib_sch->get_executor().execute(std::move(func), lifetime);
		}
	}
	fiber_executor fib_exr;
	fib_exr.execute(std::move(func), lifetime);
	fib_exr.run();
	return nullptr;
}

} // namespace rua

#endif
