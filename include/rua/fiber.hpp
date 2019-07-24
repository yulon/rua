#ifndef _RUA_FIBER_HPP
#define _RUA_FIBER_HPP

#include "bin.hpp"
#include "chrono.hpp"
#include "limits.hpp"
#include "sched.hpp"
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

	struct not_auto_attach {};
	fiber(not_auto_attach, std::function<void()> func, size_t dur = 0) :
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

	operator bool() const {
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
		bin stk;

		std::shared_ptr<std::atomic<bool>> is_cv_notified;
		std::cv_status cv_st;

		bool is_slept;
	};

	using _ctx_ptr_t = std::shared_ptr<_ctx_t>;

	_ctx_ptr_t _ctx;

	fiber(_ctx_ptr_t fc) : _ctx(std::move(fc)) {}
};

class fiber_driver {
public:
	fiber_driver() : _sch(scheduler(*this)) {
		get_ucontext(&_worker_uc_base);
	}

	fiber attach(std::function<void()> func, size_t dur = 0) {
		fiber f(fiber::not_auto_attach{}, std::move(func), dur);
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
		_fcs.emplace(std::move(f._ctx));
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
		auto orig_cv = orig_sch->make_cond_var();

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

					bool need_check_slps = true;
					for (;;) {
						if (_orig_cv_mtx.try_lock()) {
							if (_orig_cv_changed) {
								_orig_cv_changed = false;
								need_check_slps = false;
							} else {
								auto wait_ti = _cws.begin()->first;
								_orig_cv = orig_cv;
								orig_sch->cond_wait(
									orig_cv,
									_orig_cv_mtx,
									[this]() -> bool {
										return _orig_cv_changed;
									},
									ms(wait_ti - tick()).count());
								_orig_cv_changed = false;
								_orig_cv.reset();
							}
							_orig_cv_mtx.unlock();
							now = tick();
							break;
						}
						orig_sch->yield();
						now = tick();
						if (wake_ti <= now) {
							break;
						}
					}

					if (need_check_slps) {
						_check_slps(now);
					}
					_check_cws(now);

					continue;
				}

				auto wake_ti = _slps.begin()->first;
				if (wake_ti > now) {
					orig_sch->sleep(ms(wake_ti - now).count());
				} else {
					orig_sch->yield();
				}

				now = tick();
				_check_slps(now);

				continue;

			} else if (_cws.size()) {

				orig_sch->lock(_orig_cv_mtx);

				if (_orig_cv_changed) {
					_orig_cv_changed = false;
				} else {
					auto wake_ti = _cws.begin()->first;
					_orig_cv = orig_cv;
					orig_sch->cond_wait(
						orig_cv,
						_orig_cv_mtx,
						[this]() -> bool { return _orig_cv_changed; },
						ms(wake_ti - now).count());
					_orig_cv_changed = false;
					_orig_cv.reset();
				}

				_orig_cv_mtx.unlock();

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

			auto &fc =
				*slp_map.emplace(wake_ti, std::move(_fib_dvr->_cur_fc))->second;

			fc.is_slept = true;

			fc.stk = std::move(_fib_dvr->_cur_stk);

			if (_fib_dvr->_fcs.size()) {
				_fib_dvr->_swap_worker_uc(&fc.uc);
				return;
			}
			swap_ucontext(&fc.uc, &_fib_dvr->_orig_uc);
		}

		virtual void sleep(size_t timeout) {
			if (timeout > static_cast<size_t>(nmax<int64_t>())) {
				timeout = static_cast<size_t>(nmax<int64_t>());
			}
			_sleep(_fib_dvr->_slps, static_cast<int64_t>(timeout));
		}

		class cond_var : public rua::scheduler::cond_var {
		public:
			cond_var(fiber_driver &fib_dvr) :
				_fib_dvr(&fib_dvr),
				_n(std::make_shared<std::atomic<bool>>(false)) {}

			virtual void notify() {
				_n->store(true);
				_fib_dvr->_orig_cv_notify();
			}

		private:
			fiber_driver *_fib_dvr;
			std::shared_ptr<std::atomic<bool>> _n;
			friend scheduler;
		};

		virtual rua::scheduler::cond_var_i make_cond_var() {
			return std::make_shared<cond_var>(*_fib_dvr);
		}

		virtual std::cv_status cond_wait(
			rua::scheduler::cond_var_i cv,
			typeless_lock_ref &lck,
			size_t timeout = nmax<size_t>()) {

			if (timeout > static_cast<size_t>(nmax<int64_t>())) {
				timeout = static_cast<size_t>(nmax<int64_t>());
			}

			auto fscv = cv.to<cond_var>();
			assert(fscv->_fib_dvr == _fib_dvr);

			_fib_dvr->_cur_fc->is_cv_notified = fscv->_n;

			lck.unlock();
			_sleep(_fib_dvr->_cws, static_cast<int64_t>(timeout));

			lock(lck);
			return _fib_dvr->_cur_fc->cv_st;
		}

		fiber_driver *get_fiber_pool() {
			return _fib_dvr;
		}

	private:
		fiber_driver *_fib_dvr;
	};

	scheduler &get_scheduler() {
		return _sch;
	}

	size_t fiber_count() const {
		auto c = _slps.size() + _cws.size();
		if (_cur_stk) {
			++c;
		}
		return c;
	}

	size_t stack_count() const {
		auto c = _idle_stks.size();
		if (_cur_stk) {
			++c;
		}
		for (auto &pr : _slps) {
			if (pr.second->stk) {
				++c;
			}
		}
		for (auto &pr : _cws) {
			if (pr.second->stk) {
				++c;
			}
		}
		return c;
	}

private:
	std::queue<fiber::_ctx_ptr_t> _fcs;
	fiber::_ctx_ptr_t _cur_fc;

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
			if (it->second->is_cv_notified->exchange(false)) {
				it->second->cv_st = std::cv_status::no_timeout;
				_fcs.emplace(std::move(it->second));
				it = _cws.erase(it);
				continue;
			}
			if (now >= it->first) {
				it->second->cv_st = std::cv_status::timeout;
				_fcs.emplace(std::move(it->second));
				it = _cws.erase(it);
				continue;
			}
			++it;
		}
	}

	ucontext_t _orig_uc, _worker_uc_base;

	bin _cur_stk;
	std::vector<bin> _idle_stks;

	void _resume_cur_stk() {
		if (_cur_stk) {
			_idle_stks.emplace_back(std::move(_cur_stk));
		}
		_cur_stk = std::move(_cur_fc->stk);
		make_ucontext(&_worker_uc_base, &_worker, this, _cur_stk);
	}

	void _resume_cur_fc() {
		_resume_cur_stk();
		set_ucontext(&_cur_fc->uc);
	}

	void _resume_cur_fc(ucontext_t *oucp) {
		_resume_cur_stk();
		swap_ucontext(oucp, &_cur_fc->uc);
	}

	static void _worker(any_word p) {
		auto &fib_dvr = *p.to<fiber_driver *>();

		while (fib_dvr._fcs.size()) {
			fib_dvr._cur_fc = std::move(fib_dvr._fcs.front());
			fib_dvr._fcs.pop();

			if (fib_dvr._cur_fc->stk) {
				fib_dvr._resume_cur_fc();
				continue;
			}

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

	void _swap_worker_uc(ucontext_t *oucp) {
		if (!_cur_stk) {
			if (_idle_stks.empty()) {
				_cur_stk.reset(8 * 1024 * 1024);
			} else {
				_cur_stk = std::move(_idle_stks.back());
				_idle_stks.pop_back();
			}
			make_ucontext(&_worker_uc_base, &_worker, this, _cur_stk);
		}
		swap_ucontext(oucp, &_worker_uc_base);
	}

	void _work() {
		if (_fcs.front()->stk) {
			_cur_fc = std::move(_fcs.front());
			_fcs.pop();
			_resume_cur_fc(&_orig_uc);
			return;
		}
		_swap_worker_uc(&_orig_uc);
	}

	friend scheduler;
	scheduler _sch;

	bool _orig_cv_changed;
	rua::scheduler::cond_var_i _orig_cv;
	std::mutex _orig_cv_mtx;

	void _orig_cv_notify() {
		rua::get_scheduler()->lock(_orig_cv_mtx);
		_orig_cv_changed = true;
		if (_orig_cv) {
			auto orig_cv = _orig_cv;
			_orig_cv_mtx.unlock();
			orig_cv->notify();
			return;
		}
		_orig_cv_mtx.unlock();
	}
};

inline fiber::fiber(std::function<void()> func, size_t dur) :
	fiber(fiber::not_auto_attach{}, std::move(func), dur) {
	auto s = get_scheduler();
	if (s) {
		auto fs = s.to<fiber_driver::scheduler>();
		if (fs) {
			fs->get_fiber_pool()->attach(*this);
			return;
		}
	}
	std::unique_ptr<fiber_driver> fib_dvr_uptr(new fiber_driver);
	fib_dvr_uptr->attach(*this);
	fib_dvr_uptr->run();
}

inline fiber::fiber(
	fiber_driver &fib_dvr, std::function<void()> func, size_t dur) :
	fiber(fiber::not_auto_attach{}, std::move(func), dur) {
	fib_dvr.attach(*this);
}

} // namespace rua

#endif
