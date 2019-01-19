#ifndef _RUA_FIBER_HPP
#define _RUA_FIBER_HPP

#include "bin.hpp"
#include "ucontext.hpp"
#include "sched.hpp"
#include "limits.hpp"

#include <functional>
#include <list>
#include <queue>
#include <map>
#include <memory>
#include <chrono>
#include <array>
#include <cassert>

namespace rua {

inline size_t _fiber_now() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock().now().time_since_epoch()).count();
}

class fiber_pool;

class fiber {
public:
	fiber() = default;

	inline fiber(std::function<void()> func, size_t dur = 0, fiber_pool *fp_ptr = nullptr);

	struct not_run {};
	inline fiber(not_run, std::function<void()> func, size_t dur = 0) {
		_ctx = std::make_shared<_ctx_t>();
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
	friend fiber_pool;

	struct _ctx_t {
		std::function<void()> fn;

		std::atomic<bool> is_stoped;
		size_t end_ti;

		void reset_duration(size_t dur = 0) {
			if (!dur) {
				end_ti = 0;
			}
			auto now = _fiber_now();
			if (dur >= nmax<size_t>() - now) {
				end_ti = nmax<size_t>();
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

	fiber(_ctx_ptr_t &&fc) : _ctx(std::move(fc)) {}
	fiber(const _ctx_ptr_t &fc) : _ctx(fc) {}
};

class fiber_pool {
public:
	fiber_pool() : _sch(scheduler(*this)) {
		get_ucontext(&_worker_uc_base);
	}

	fiber add(std::function<void()> func, size_t dur = 0) {
		fiber f(fiber::not_run{}, std::move(func), dur);
		f._ctx->is_stoped = false;
		_fcs.emplace(f._ctx);
		return f;
	}

	void add(fiber f) {
		assert(f._ctx);

		auto old = f._ctx->is_stoped.exchange(false);
		if (!old) {
			return;
		}
		_fcs.emplace(std::move(f._ctx));
	}

	/*void enable_add_from_other_thread() {
		assert(false);
	}

	void add_from_other_thread(std::function<void()> func, size_t dur = 0) {
		assert(false);
	}*/

	fiber current() const {
		return _cur_fc;
	}

	void step() {
		auto now = _fiber_now();

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

	void run() {
		if (_fcs.empty() && _slps.empty() && _cws.empty()) {
			return;
		}

		scheduler_guard sg(_sch);
		auto ll_sch = sg.previous();
		auto ll_cv = ll_sch->make_cond_var();

		auto now = _fiber_now();

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

			now = _fiber_now();

			if (_slps.size()) {
				if (_cws.size()) {

					size_t wake_ti;
					if (_cws.begin()->first < _slps.begin()->first) {
						wake_ti = _cws.begin()->first;
					} else {
						wake_ti = _slps.begin()->first;
					}

					bool need_check_slps = true;
					for (;;) {
						if (_ll_cv_mtx.try_lock()) {
							if (_ll_cv_changed) {
								_ll_cv_changed = false;
								need_check_slps = false;
							} else {
								auto wake_ti = _cws.begin()->first;
								_ll_cv = ll_cv;
								ll_sch->cond_wait(ll_cv, _ll_cv_mtx, [this]() -> bool {
									return _ll_cv_changed;
								}, wake_ti - _fiber_now());
								_ll_cv_changed = false;
								_ll_cv.reset();
							}
							_ll_cv_mtx.unlock();
							now = _fiber_now();
							break;
						}
						ll_sch->yield();
						now = _fiber_now();
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
					ll_sch->sleep(wake_ti - now);
				} else {
					ll_sch->yield();
				}

				now = _fiber_now();
				_check_slps(now);

				continue;

			} else if (_cws.size()) {

				ll_sch->lock(_ll_cv_mtx);

				if (_ll_cv_changed) {
					_ll_cv_changed = false;
				} else {
					auto wake_ti = _cws.begin()->first;
					_ll_cv = ll_cv;
					ll_sch->cond_wait(ll_cv, _ll_cv_mtx, [this]() -> bool {
						return _ll_cv_changed;
					}, wake_ti - now);
					_ll_cv_changed = false;
					_ll_cv.reset();
				}

				_ll_cv_mtx.unlock();

				_check_cws(now);

			} else {
				return;
			}
		}
	}

	class scheduler : public rua::scheduler {
	public:
		scheduler() = default;

		scheduler(fiber_pool &fp) : _fp(&fp) {}

		virtual ~scheduler() = default;

		template <typename SlpMap>
		void _sleep(SlpMap &&slp_map, size_t timeout) {
			size_t wake_ti;
			if (!timeout) {
				wake_ti = 0;
			} else if (timeout >= nmax<size_t>() - _fiber_now()) {
				wake_ti = nmax<size_t>();
			} else {
				wake_ti = _fiber_now() + timeout;
			}

			auto &fc = *slp_map.emplace(
				wake_ti,
				std::move(_fp->_cur_fc)
			)->second;

			fc.is_slept = true;

			fc.stk = std::move(_fp->_cur_stk);

			if (_fp->_fcs.size()) {
				_fp->_swap_worker_uc(&fc.uc);
				return;
			}
			swap_ucontext(&fc.uc, &_fp->_orig_uc);
		}

		virtual void sleep(size_t timeout) {
			_sleep(_fp->_slps, timeout);
		}

		class cond_var : public rua::scheduler::cond_var {
		public:
			cond_var(fiber_pool &fp) : _fp(&fp), _n(std::make_shared<std::atomic<bool>>(false)) {}

			virtual void notify() {
				_n->store(true);
				_fp->_ll_cv_notify();
			}

		private:
			fiber_pool *_fp;
			std::shared_ptr<std::atomic<bool>> _n;
			friend scheduler;
		};

		virtual rua::scheduler::cond_var_i make_cond_var() {
			return std::make_shared<cond_var>(*_fp);
		}

		virtual std::cv_status cond_wait(rua::scheduler::cond_var_i cv, typeless_lock_ref &lck, size_t timeout = nmax<size_t>()) {
			auto fscv = cv.to<cond_var>();
			assert(fscv->_fp == _fp);

			_fp->_cur_fc->is_cv_notified = fscv->_n;

			lck.unlock();
			_sleep(_fp->_cws, timeout);

			lock(lck);
			return _fp->_cur_fc->cv_st;
		}

		fiber_pool *get_fiber_pool() {
			return _fp;
		}

	private:
		fiber_pool *_fp;
	};

	scheduler &get_scheduler() {
		return _sch;
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

	std::multimap<size_t, fiber::_ctx_ptr_t> _slps;
	std::multimap<size_t, fiber::_ctx_ptr_t> _cws;

	void _check_slps(size_t now) {
		for (auto it = _slps.begin(); it != _slps.end(); it = _slps.erase(it)) {
			if (now < it->first) {
				break;
			}
			_fcs.emplace(std::move(it->second));
		}
	}

	void _check_cws(size_t now) {
		for (auto it = _cws.begin(); it != _cws.end(); ) {
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
		auto &fp = *p.to<fiber_pool *>();

		while (fp._fcs.size()) {
			fp._cur_fc = std::move(fp._fcs.front());
			fp._fcs.pop();

			if (fp._cur_fc->stk) {
				fp._resume_cur_fc();
				continue;
			}

			if (fp._cur_fc->is_stoped) {
				continue;
			}

			for (;;) {
				fp._cur_fc->fn();

				if (fp._cur_fc->is_stoped) {
					break;
				}

				if (fp._cur_fc->end_ti <= _fiber_now()) {
					fp._cur_fc->is_stoped = false;
					break;
				}

				if (!fp._cur_fc->is_slept) {
					fp._slps.emplace(
						0,
						std::move(fp._cur_fc)
					);
					break;
				}
				fp._cur_fc->is_slept = false;
			}
		}

		set_ucontext(&fp._orig_uc);
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

	bool _ll_cv_changed;
	rua::scheduler::cond_var_i _ll_cv;
	std::mutex _ll_cv_mtx;

	void _ll_cv_notify() {
		rua::get_scheduler()->lock(_ll_cv_mtx);
		_ll_cv_changed = true;
		if (_ll_cv) {
			auto ll_cv = _ll_cv;
			_ll_cv_mtx.unlock();
			ll_cv->notify();
			return;
		}
		_ll_cv_mtx.unlock();
	}
};

inline fiber::fiber(std::function<void()> func, size_t dur, fiber_pool *fp_ptr) :
	fiber(fiber::not_run{}, std::move(func), dur)
{
	if (fp_ptr) {
		fp_ptr->add(*this);
		return;
	}
	auto s = get_scheduler();
	if (s) {
		auto fs = s.to<fiber_pool::scheduler>();
		if (fs) {
			fs->get_fiber_pool()->add(*this);
			return;
		}
	}
	std::unique_ptr<fiber_pool> fp_uptr(new fiber_pool);
	fp_uptr->add(*this);
	fp_uptr->run();
}

}

#endif
