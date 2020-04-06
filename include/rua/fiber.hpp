#ifndef _RUA_FIBER_HPP
#define _RUA_FIBER_HPP

#include "bytes.hpp"
#include "chrono.hpp"
#include "sched.hpp"
#include "types/util.hpp"
#include "ucontext.hpp"

#include <array>
#include <atomic>
#include <cassert>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <queue>

namespace rua {

class fiber_executor;

class fiber {
public:
	constexpr fiber() = default;

	operator bool() const {
		return _ctx && !_ctx->is_stoped.load();
	}

	void reset_lifetime(ms dur = 0) {
		if (!_ctx) {
			return;
		}
		if (!dur) {
			_ctx->end_ti.reset();
		}
		auto now = tick();
		if (dur >= time_max() - now) {
			_ctx->end_ti = time_max();
			return;
		}
		_ctx->end_ti = now + dur;
	}

	void stop() {
		if (!_ctx) {
			return;
		}
		_ctx->is_stoped.store(true);
		_ctx.reset();
	}

private:
	struct _ctx_t {
		std::function<void()> tsk;

		std::atomic<bool> is_stoped;
		time end_ti;

		ucontext_t _uc;
		int stk_ix;
		bytes stk_bak;

		bool has_yielded;
		std::shared_ptr<secondary_waker> wkr;
	};

	std::shared_ptr<_ctx_t> _ctx;

	fiber(std::shared_ptr<_ctx_t> ctx) : _ctx(std::move(ctx)) {}

	friend fiber_executor;
};

class fiber_executor {
public:
	fiber_executor(size_t stack_size = 0x100000) :
		_stk_sz(stack_size), _stk_ix(0), _sch(*this) {}

	fiber execute(std::function<void()> task, ms lifetime = 0) {
		fiber f(std::make_shared<fiber::_ctx_t>());
		f._ctx->tsk = std::move(task);
		f._ctx->is_stoped.store(false);
		f.reset_lifetime(lifetime);
		_exs.emplace(f);
		return f;
	}

	fiber executing() const {
		return _cur;
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
		_switch_to_runner_uc();
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
			_switch_to_runner_uc();

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

			_fe->_cur._ctx->has_yielded = true;
			_fe->_cur._ctx->stk_ix = _fe->_stk_ix;
			slp_map.emplace(wake_ti, _fe->_cur);

			_fe->_prev = std::move(_fe->_cur);
			if (_fe->_exs.size()) {
				if (!_fe->_try_resume_exs_front()) {
					_fe->_swap_new_runner_uc(&_fe->_prev._ctx->_uc);
				}
			} else {
				swap_ucontext(&_fe->_prev._ctx->_uc, &_fe->_orig_uc);
			}
			_fe->_clear_prev();
		}

		virtual bool sleep(ms timeout, bool wakeable = false) {
			if (!wakeable) {
				_sleep(_fe->_slps, timeout);
				return false;
			}

			auto &wkr = _fe->_cur._ctx->wkr;
			if (!wkr->state()) {
				_sleep(_fe->_cws, timeout);
			}
			return wkr->state();
		}

		virtual waker_i get_waker() {
			assert(_fe->_cur._ctx);

			auto &wkr = _fe->_cur._ctx->wkr;
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
	std::queue<fiber> _exs;
	fiber _cur, _prev;

	// TODO: time wheel
	std::multimap<time, fiber> _slps;
	std::multimap<time, fiber> _cws;

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
			if (it->second._ctx->wkr->state() || now >= it->first) {
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
	ucontext_t _new_runner_ucs[2];

	bytes &_cur_stk() {
		return _stks[_stk_ix];
	}

	ucontext_t &_cur_new_runner_uc() {
		return _new_runner_ucs[_stk_ix];
	}

	void _clear_prev() {
		if (!_prev) {
			return;
		}

		assert(!_prev._ctx->stk_bak.size());

		auto &stk = _stks[_prev._ctx->stk_ix];
		auto stk_used = stk(_prev._ctx->_uc.sp() - stk.data().uintptr());
		auto rmdr = stk_used.size() % 1024;
		_prev._ctx->stk_bak.resize(stk_used.size() + (rmdr ? 1024 - rmdr : 0));
		_prev._ctx->stk_bak = stk_used;

		_prev._ctx.reset();
	}

	bool _try_resume_exs_front(ucontext_t *oucp = nullptr) {
		if (!_exs.front()._ctx->stk_bak.size()) {
			return false;
		}

		if (oucp != &_orig_uc && _exs.front()._ctx->stk_ix == _stk_ix) {
			if (!oucp) {
				set_ucontext(&_orig_uc);
				return true;
			}
			swap_ucontext(oucp, &_orig_uc);
			return true;
		}

		_cur = std::move(_exs.front());
		_exs.pop();

		_stk_ix = _cur._ctx->stk_ix;
		auto &cur_stk = _cur_stk();
		cur_stk(cur_stk.size() - _cur._ctx->stk_bak.size())
			.copy_from(_cur._ctx->stk_bak);
		_cur._ctx->stk_bak.resize(0);

		if (!oucp) {
			set_ucontext(&_cur._ctx->_uc);
			return true;
		}
		swap_ucontext(oucp, &_cur._ctx->_uc);
		return true;
	}

	void _run() {
		_clear_prev();

		while (_exs.size()) {
			assert(_exs.front()._ctx);
			_try_resume_exs_front();

			_cur = std::move(_exs.front());
			assert(!_exs.front()._ctx);
			_exs.pop();

			if (_cur._ctx->is_stoped.load()) {
				continue;
			}

			for (;;) {
				_cur._ctx->tsk();

				if (_cur._ctx->is_stoped.load()) {
					break;
				}

				if (_cur._ctx->end_ti <= tick()) {
					_cur._ctx->is_stoped.store(false);
					break;
				}

				if (!_cur._ctx->has_yielded) {
					_slps.emplace(time_zero(), std::move(_cur));
					break;
				}
				_cur._ctx->has_yielded = false;
			}
		}

		set_ucontext(&_orig_uc);
	}

	static void _runner(any_word th1s) {
		th1s.as<fiber_executor *>()->_run();
	}

	void _swap_new_runner_uc(ucontext_t *oucp) {
		if (oucp != &_orig_uc) {
			_stk_ix = !_stk_ix;
		}
		auto &cur_stk = _cur_stk();
		if (!cur_stk) {
			cur_stk.reset(_stk_sz);
			get_ucontext(&_cur_new_runner_uc());
			make_ucontext(&_cur_new_runner_uc(), &_runner, this, cur_stk);
		}
		swap_ucontext(oucp, &_cur_new_runner_uc());
	}

	void _switch_to_runner_uc() {
		while (_exs.size()) {
			if (!_try_resume_exs_front(&_orig_uc)) {
				_swap_new_runner_uc(&_orig_uc);
			}
			_clear_prev();
		}
	}

	friend scheduler;
	scheduler _sch;

	waker_i _orig_wkr;
};

inline fiber_executor *this_fiber_executor() {
	auto sch = this_scheduler();
	if (!sch) {
		return nullptr;
	}
	auto fs = sch.as<fiber_executor::scheduler>();
	if (!fs) {
		return nullptr;
	}
	return &fs->get_executor();
}

inline fiber this_fiber() {
	auto fe = this_fiber_executor();
	if (fe) {
		return fe->executing();
	}
	return fiber();
}

inline fiber co(std::function<void()> task, ms lifetime = 0) {
	auto fe = this_fiber_executor();
	if (fe) {
		return fe->execute(std::move(task), lifetime);
	}
	auto tmp_fe = std::make_shared<fiber_executor>();
	tmp_fe->execute(std::move(task), lifetime);
	tmp_fe->run();
	return fiber();
}

} // namespace rua

#endif
