#ifndef _RUA_FIBER_HPP
#define _RUA_FIBER_HPP

#include "bytes.hpp"
#include "chrono.hpp"
#include "limits.hpp"
#include "sched/util.hpp"
#include "ucontext.hpp"

#include <array>
#include <cassert>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <queue>

namespace rua {

class fiber_driver;

class fiber {
public:
	fiber() = default;

	inline fiber(std::function<void()> func, size_t dur = 0);

	inline fiber(
		fiber_driver &fib_dvr, std::function<void()> func, size_t dur = 0);

	struct not_auto_attach_t {};
	static constexpr not_auto_attach_t not_auto_attach{};

	fiber(not_auto_attach_t, std::function<void()> func, size_t dur = 0) :
		_ctx(std::make_shared<_ctx_t>()) {
		_ctx->fn = std::move(func);
		_ctx->is_stoped = true;
		_ctx->reset_duration(dur);
	}

	void stop() {
		if (!_ctx) {
			return;
		}
		_ctx->is_stoped = true;
		_ctx.reset();
	}

	void reset_duration(size_t dur = 0) {
		if (!_ctx) {
			return;
		}
		if (!_ctx->is_stoped) {
			_ctx.reset();
			return;
		}
		_ctx->reset_duration(dur);
	}

	explicit operator bool() const {
		return _ctx && !_ctx->is_stoped;
	}

private:
	friend fiber_driver;

	struct _ctx_t {
		std::function<void()> fn;

		std::atomic<bool> is_stoped;
		time end_ti;

		void reset_duration(ms dur = 0) {
			if (!dur) {
				end_ti.reset();
			}
			auto now = tick();
			if (dur >= time_max() - now) {
				end_ti = time_max();
				return;
			}
			end_ti = now + dur;
		}

		ucontext_t uc;
		int stk_ix;
		bytes stk_data;

		bool is_slept;

		scheduler::signaler_i sig;
	};

	using _ctx_ptr_t = std::shared_ptr<_ctx_t>;

	_ctx_ptr_t _ctx;

	fiber(_ctx_ptr_t fc) : _ctx(std::move(fc)) {}
};

class fiber_driver {
public:
	fiber_driver(size_t stack_size = 0x100000) :
		_stk_sz(stack_size),
		_stk_ix(0),
		_sch(*this) {}

	fiber attach(std::function<void()> func, size_t dur = 0) {
		fiber f(fiber::not_auto_attach, std::move(func), dur);
		f._ctx->is_stoped = false;
		_fcs.emplace(f._ctx);
		return f;
	}

	void attach(fiber f) {
		assert(f._ctx);

		auto old = f._ctx->is_stoped.exchange(false);
		if (!old) {
			return;
		}
		_fcs.emplace(f._ctx);
	}

	fiber current() const {
		return _cur_fc;
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
		_work();
	}

	// May block the current context.
	// The current scheduler will be used.
	void run() {
		if (_fcs.empty() && _slps.empty() && _cws.empty()) {
			return;
		}

		scheduler_guard sg(_sch);
		auto orig_sch = sg.previous();
		_orig_sig = orig_sch->get_signaler();

		auto now = tick();

		if (_slps.size()) {
			_check_slps(now);
		}
		if (_cws.size()) {
			_check_cws(now);
		}

		for (;;) {
			if (_fcs.size()) {
				_work();
			}

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
						orig_sch->wait(wake_ti - now);
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
					orig_sch->wait(wake_ti - now);
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

		scheduler(fiber_driver &fib_dvr) : _fib_dvr(&fib_dvr) {}

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

			auto &fc = *slp_map.emplace(wake_ti, _fib_dvr->_cur_fc)->second;

			fc.is_slept = true;
			fc.stk_ix = _fib_dvr->_stk_ix;

			_fib_dvr->_prev_fc = std::move(_fib_dvr->_cur_fc);

			if (_fib_dvr->_fcs.size()) {
				if (!_fib_dvr->_try_resume_fcs_front()) {
					_fib_dvr->_swap_new_worker_uc(&fc.uc);
				}
			} else {
				swap_ucontext(&fc.uc, &_fib_dvr->_orig_uc);
			}

			_fib_dvr->_save_prev_fc_stk_data();
		}

		virtual void sleep(ms timeout) {
			_sleep(_fib_dvr->_slps, timeout);
		}

		using signaler = rua::secondary_signaler;

		virtual signaler_i get_signaler() {
			assert(_fib_dvr->_cur_fc);

			if (!_fib_dvr->_cur_fc->sig.type_is<signaler>() ||
				_fib_dvr->_cur_fc->sig.as<signaler>()->primary() !=
					_fib_dvr->_orig_sig) {
				_fib_dvr->_cur_fc->sig =
					std::make_shared<signaler>(_fib_dvr->_orig_sig);
			}
			return _fib_dvr->_cur_fc->sig;
		}

		virtual bool wait(ms timeout = duration_max()) {
			assert(_fib_dvr->_cur_fc->sig.type_is<signaler>());
			assert(_fib_dvr->_cur_fc->sig.as<signaler>()->primary());

			auto sig = _fib_dvr->_cur_fc->sig.as<signaler>();
			auto state = sig->state();
			if (!state) {
				_sleep(_fib_dvr->_cws, timeout);
				state = sig->state();
			}
			sig->reset();
			return state;
		}

		fiber_driver *get_fiber_driver() {
			return _fib_dvr;
		}

	private:
		fiber_driver *_fib_dvr;
	};

	scheduler &get_scheduler() {
		return _sch;
	}

private:
	std::queue<fiber::_ctx_ptr_t> _fcs;
	fiber::_ctx_ptr_t _cur_fc, _prev_fc;

	std::multimap<time, fiber::_ctx_ptr_t> _slps;
	std::multimap<time, fiber::_ctx_ptr_t> _cws;

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
			if (it->second->sig.as<secondary_signaler>()->state() ||
				now >= it->first) {
				_fcs.emplace(std::move(it->second));
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

	void _save_prev_fc_stk_data() {
		if (!_prev_fc) {
			return;
		}
		assert(!_prev_fc->stk_data.size());
		auto &stk = _stks[_prev_fc->stk_ix];
		_prev_fc->stk_data =
			stk(_prev_fc->uc.regs.sp + sizeof(void *) - stk.data());
		_prev_fc.reset();
	}

	bool _try_resume_fcs_front(ucontext_t *oucp = nullptr) {
		if (!_fcs.front()->stk_data.size()) {
			return false;
		}

		if (oucp != &_orig_uc && _fcs.front()->stk_ix == _stk_ix) {
			if (!oucp) {
				set_ucontext(&_orig_uc);
				return true;
			}
			swap_ucontext(oucp, &_orig_uc);
			return true;
		}

		_cur_fc = std::move(_fcs.front());
		_fcs.pop();

		_stk_ix = _cur_fc->stk_ix;
		_cur_stk()(_cur_stk().size() - _cur_fc->stk_data.size())
			.copy_from(_cur_fc->stk_data);
		_cur_fc->stk_data.reset();

		if (!oucp) {
			set_ucontext(&_cur_fc->uc);
			return true;
		}
		swap_ucontext(oucp, &_cur_fc->uc);
		return true;
	}

	static void _worker(any_word p) {
		auto &fib_dvr = *p.as<fiber_driver *>();

		fib_dvr._save_prev_fc_stk_data();

		while (fib_dvr._fcs.size()) {
			assert(fib_dvr._fcs.front());
			fib_dvr._try_resume_fcs_front();

			fib_dvr._cur_fc = std::move(fib_dvr._fcs.front());
			assert(!fib_dvr._fcs.front());
			fib_dvr._fcs.pop();

			if (fib_dvr._cur_fc->is_stoped) {
				continue;
			}

			for (;;) {
				fib_dvr._cur_fc->fn();

				if (fib_dvr._cur_fc->is_stoped) {
					break;
				}

				if (fib_dvr._cur_fc->end_ti <= tick()) {
					fib_dvr._cur_fc->is_stoped = false;
					break;
				}

				if (!fib_dvr._cur_fc->is_slept) {
					fib_dvr._slps.emplace(
						time_zero(), std::move(fib_dvr._cur_fc));
					break;
				}
				fib_dvr._cur_fc->is_slept = false;
			}
		}

		set_ucontext(&fib_dvr._orig_uc);
	}

	void _swap_new_worker_uc(ucontext_t *oucp) {
		if (oucp != &_orig_uc) {
			_stk_ix = !_stk_ix;
		}
		if (!_cur_stk()) {
			_cur_stk().reset(_stk_sz);
			get_ucontext(&_cur_new_worker_uc());
			make_ucontext(&_cur_new_worker_uc(), &_worker, this, _cur_stk());
		}
		swap_ucontext(oucp, &_cur_new_worker_uc());
	}

	void _work() {
		while (_fcs.size()) {
			if (!_try_resume_fcs_front(&_orig_uc)) {
				_swap_new_worker_uc(&_orig_uc);
			}
			_save_prev_fc_stk_data();
		}
	}

	friend scheduler;
	scheduler _sch;

	rua::scheduler::signaler_i _orig_sig;
};

inline fiber::fiber(std::function<void()> func, size_t dur) :
	fiber(fiber::not_auto_attach, std::move(func), dur) {
	auto sch = this_scheduler();
	if (sch) {
		auto fsch = sch.as<fiber_driver::scheduler>();
		if (fsch) {
			fsch->get_fiber_driver()->attach(*this);
			return;
		}
	}
	std::unique_ptr<fiber_driver> fib_dvr_uptr(new fiber_driver);
	fib_dvr_uptr->attach(*this);
	fib_dvr_uptr->run();
}

inline fiber::fiber(
	fiber_driver &fib_dvr, std::function<void()> func, size_t dur) :
	fiber(fiber::not_auto_attach, std::move(func), dur) {
	fib_dvr.attach(*this);
}

} // namespace rua

#endif
