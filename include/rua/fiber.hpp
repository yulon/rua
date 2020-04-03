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

class fiber_executor;

class _fctx_t {
private:
	std::function<void()> tsk;

	bool is_stoped;
	time end_ti;

	ucontext_t _uc;
	int stk_ix;
	bytes stk_bak;

	bool has_yielded;
	std::shared_ptr<secondary_waker> wkr;

	friend fiber_executor;
};

class fid_t {
public:
	constexpr fid_t() = default;

	operator bool() const {
		return _p.get();
	}

private:
	std::shared_ptr<_fctx_t> _p;

	fid_t(std::shared_ptr<_fctx_t> p) : _p(std::move(p)) {}

	friend fiber_executor;
};

class fiber_executor {
public:
	fiber_executor(size_t stack_size = 0x100000) :
		_stk_sz(stack_size), _stk_ix(0), _sch(*this) {}

	fid_t execute(std::function<void()> func, ms lifetime = 0) {
		fid_t ctx(std::make_shared<_fctx_t>());
		ctx._p->tsk = std::move(func);
		ctx._p->is_stoped = false;
		reset_lifetime(ctx, lifetime);
		_exs.emplace(ctx);
		return ctx;
	}

	fid_t current() const {
		return _cur_ctx;
	}

	void reset_lifetime(fid_t ctx, ms dur = 0) {
		if (!dur) {
			ctx._p->end_ti.reset();
		}
		auto now = tick();
		if (dur >= time_max() - now) {
			ctx._p->end_ti = time_max();
			return;
		}
		ctx._p->end_ti = now + dur;
	}

	void unexecute(fid_t ctx) {
		ctx._p->is_stoped = true;
	}

	operator bool() const {
		return _exs.size() || _slps.size() || _cws.size();
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
		if (_exs.empty()) {
			return;
		}

		scheduler_guard sg(_sch);
		_switch_to_worker_uc();
	}

	// May block the current context.
	// The current scheduler will be used.
	void run() {
		if (_exs.empty() && _slps.empty() && _cws.empty()) {
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

		scheduler(fiber_executor &fe) : _fe(&fe) {}

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

			_fe->_cur_ctx._p->has_yielded = true;
			_fe->_cur_ctx._p->stk_ix = _fe->_stk_ix;
			slp_map.emplace(wake_ti, _fe->_cur_ctx);

			_fe->_prev_ctx = std::move(_fe->_cur_ctx);
			if (_fe->_exs.size()) {
				if (!_fe->_try_resume_ctxs_front()) {
					_fe->_swap_new_worker_uc(&_fe->_prev_ctx._p->_uc);
				}
			} else {
				swap_ucontext(&_fe->_prev_ctx._p->_uc, &_fe->_orig_uc);
			}
			_fe->_clear_prev_ctx();
		}

		virtual bool sleep(ms timeout, bool wakeable = false) {
			if (!wakeable) {
				_sleep(_fe->_slps, timeout);
				return false;
			}

			auto &wkr = _fe->_cur_ctx._p->wkr;
			if (!wkr->state()) {
				_sleep(_fe->_cws, timeout);
			}
			return wkr->state();
		}

		virtual waker_i get_waker() {
			assert(_fe->_cur_ctx);

			auto &wkr = _fe->_cur_ctx._p->wkr;
			if (wkr) {
				wkr.reset();
			} else {
				wkr = std::make_shared<secondary_waker>(_fe->_orig_wkr);
			}
			return wkr;
		}

		fiber_executor &get_executor() {
			return *_fe;
		}

	private:
		fiber_executor *_fe;
	};

	scheduler &get_scheduler() {
		return _sch;
	}

private:
	std::queue<fid_t> _exs;
	fid_t _cur_ctx, _prev_ctx;

	std::multimap<time, fid_t> _slps;
	std::multimap<time, fid_t> _cws;

	void _check_slps(time now) {
		for (auto it = _slps.begin(); it != _slps.end(); it = _slps.erase(it)) {
			if (now < it->first) {
				break;
			}
			_exs.emplace(std::move(it->second));
		}
	}

	void _check_cws(time now) {
		for (auto it = _cws.begin(); it != _cws.end();) {
			if (it->second._p->wkr->state() || now >= it->first) {
				_exs.emplace(std::move(it->second));
				it = _cws.erase(it);
				continue;
			}
			++it;
		}
	}

	ucontext_t _orig_uc;

	size_t _stk_sz;
	int _stk_ix;
	bytes _stks[2];
	ucontext_t _new_worker_ucs[2];

	bytes &_cur_stk() {
		return _stks[_stk_ix];
	}

	ucontext_t &_cur_new_worker_uc() {
		return _new_worker_ucs[_stk_ix];
	}

	void _clear_prev_ctx() {
		if (!_prev_ctx) {
			return;
		}

		assert(!_prev_ctx._p->stk_bak.size());

		auto &stk = _stks[_prev_ctx._p->stk_ix];
		auto stk_used = stk(_prev_ctx._p->_uc.sp() - stk.data().uintptr());
		auto rmdr = stk_used.size() % 1024;
		_prev_ctx._p->stk_bak.resize(
			stk_used.size() + (rmdr ? 1024 - rmdr : 0));
		_prev_ctx._p->stk_bak = stk_used;

		_prev_ctx._p.reset();
	}

	bool _try_resume_ctxs_front(ucontext_t *oucp = nullptr) {
		if (!_exs.front()._p->stk_bak.size()) {
			return false;
		}

		if (oucp != &_orig_uc && _exs.front()._p->stk_ix == _stk_ix) {
			if (!oucp) {
				set_ucontext(&_orig_uc);
				return true;
			}
			swap_ucontext(oucp, &_orig_uc);
			return true;
		}

		_cur_ctx = std::move(_exs.front());
		_exs.pop();

		_stk_ix = _cur_ctx._p->stk_ix;
		auto &cur_stk = _cur_stk();
		cur_stk(cur_stk.size() - _cur_ctx._p->stk_bak.size())
			.copy_from(_cur_ctx._p->stk_bak);
		_cur_ctx._p->stk_bak.resize(0);

		if (!oucp) {
			set_ucontext(&_cur_ctx._p->_uc);
			return true;
		}
		swap_ucontext(oucp, &_cur_ctx._p->_uc);
		return true;
	}

	void _work() {
		_clear_prev_ctx();

		while (_exs.size()) {
			assert(_exs.front());
			_try_resume_ctxs_front();

			_cur_ctx = std::move(_exs.front());
			assert(!_exs.front());
			_exs.pop();

			if (_cur_ctx._p->is_stoped) {
				continue;
			}

			for (;;) {
				_cur_ctx._p->tsk();

				if (_cur_ctx._p->is_stoped) {
					break;
				}

				if (_cur_ctx._p->end_ti <= tick()) {
					_cur_ctx._p->is_stoped = false;
					break;
				}

				if (!_cur_ctx._p->has_yielded) {
					_slps.emplace(time_zero(), std::move(_cur_ctx));
					break;
				}
				_cur_ctx._p->has_yielded = false;
			}
		}

		set_ucontext(&_orig_uc);
	}

	static void _worker(any_word th1s) {
		th1s.as<fiber_executor *>()->_work();
	}

	void _swap_new_worker_uc(ucontext_t *oucp) {
		if (oucp != &_orig_uc) {
			_stk_ix = !_stk_ix;
		}
		auto &cur_stk = _cur_stk();
		if (!cur_stk) {
			cur_stk.reset(_stk_sz);
			get_ucontext(&_cur_new_worker_uc());
			make_ucontext(&_cur_new_worker_uc(), &_worker, this, cur_stk);
		}
		swap_ucontext(oucp, &_cur_new_worker_uc());
	}

	void _switch_to_worker_uc() {
		while (_exs.size()) {
			if (!_try_resume_ctxs_front(&_orig_uc)) {
				_swap_new_worker_uc(&_orig_uc);
			}
			_clear_prev_ctx();
		}
	}

	friend scheduler;
	scheduler _sch;

	waker_i _orig_wkr;
};

inline fid_t co(std::function<void()> func, ms lifetime = 0) {
	auto sch = this_scheduler();
	if (sch) {
		auto fib_sch = sch.as<fiber_executor::scheduler>();
		if (fib_sch) {
			return fib_sch->get_executor().execute(std::move(func), lifetime);
		}
	}
	auto fib_exr = std::make_shared<fiber_executor>();
	fib_exr->execute(std::move(func), lifetime);
	fib_exr->run();
	return fid_t();
}

} // namespace rua

#endif
