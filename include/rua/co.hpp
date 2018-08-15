#ifndef _RUA_CO_HPP
#define _RUA_CO_HPP

#include "bin.hpp"
#include "cont.hpp"
#include "sched.hpp"
#include "limits.hpp"

#include <functional>
#include <list>
#include <memory>
#include <chrono>
#include <array>
#include <cassert>

namespace rua {
	class co_pool {
		public:
			co_pool() : _need_ll_sleep(false), _need_ll_cw(0), _sch(_scheduler(*this)) {
				_cur_tsk_it = _tsks.end();
				_cur_insert_it = _tsks.begin();
				_cur_tskr_ct.save();
			}

			void add(std::function<void()> task, size_t timeout = 0) {
				auto it = _tsks.insert(_cur_insert_it, _tsk_ctx_t{
					std::move(task),
					timeout > 0,
					0,
					nullptr,
					nullptr,
					0,
					false,
					nullptr,
					std::cv_status::no_timeout
				});
				if (!timeout) {
					return;
				}
				if (timeout >= nmax<size_t>() - _cur_ti()) {
					it->rept_end_ti = nmax<size_t>();
					return;
				}
				it->rept_end_ti = _cur_ti() + timeout;
			}

			void run() {
				scheduler_guard sg(_sch);
				auto &ll_sch = sg.previous();
				auto ll_cv = ll_sch.make_cond_var();

				while (_tsks.size()) {
					if (_need_ll_sleep) {
						auto cur_ti = _cur_ti();
						if (_ll_wake_ti <= cur_ti) {
							ll_sch.yield();
						} else if (_need_ll_cw) {
							for (;;) {
								if (_ll_cv_mtx.try_lock()) {
									if (_ll_cv_changed) {
										_ll_cv_changed = false;
										_ll_cv_mtx.unlock();
										ll_sch.yield();
										break;
									}
									_ll_cv = ll_cv;
									ll_sch.cond_wait(ll_cv, _ll_cv_mtx, [this]() -> bool {
										return _ll_cv_changed;
									}, _ll_wake_ti - cur_ti);
									_ll_cv_changed = false;
									_ll_cv.reset();
									_ll_cv_mtx.unlock();
									break;
								}
								ll_sch.yield();
								cur_ti = _cur_ti();
								if (_ll_wake_ti <= cur_ti) {
									break;
								}
							}
						} else {
							ll_sch.sleep(_ll_wake_ti - cur_ti);
						}
						_need_ll_sleep = false;
					}
					_ll_wake_ti = nmax<size_t>();

					_reset_cur_tsk_it();
					_new_tskr_co(_orig_ct);
				}
			}

			void run_onec_frame() {
				if (_tsks.empty()) {
					return;
				}

				if (_need_ll_sleep) {
					if (_ll_wake_ti > _cur_ti()) {
						if (_need_ll_cw) {
							if (!_ll_cv_mtx.try_lock()) {
								return;
							}
							if (!_ll_cv_changed) {
								_ll_cv_mtx.unlock();
								return;
							}
							_ll_cv_mtx.unlock();
						} else {
							return;
						}
					}
					_need_ll_sleep = false;
				}

				scheduler_guard sg(_sch);
				_ll_wake_ti = nmax<size_t>();

				_reset_cur_tsk_it();
				_new_tskr_co(_orig_ct);
			}

			scheduler &get_scheduler() {
				return _sch;
			}

		private:
			struct _tsk_ctx_t {
				std::function<void()> fn;

				bool is_rept;
				size_t rept_end_ti;

				cont ct;
				bin stk;

				bool is_sleeping() const {
					return stk;
				}

				size_t wake_ti;
				bool is_cw;
				std::shared_ptr<std::atomic<bool>> is_cv_notified;
				std::cv_status cv_st;
			};

			std::list<_tsk_ctx_t> _tsks, _adding_befores, _adding_afters;
			std::list<_tsk_ctx_t>::iterator _cur_tsk_it, _cur_insert_it;

			void _reset_cur_tsk_it() {
				_cur_tsk_it = _tsks.begin();
				return _reset_cur_insert_it();
			}

			void _cur_tsk_it_inc() {
				++_cur_tsk_it;
				return _reset_cur_insert_it();
			}

			void _set_cur_tsk_it(std::list<_tsk_ctx_t>::iterator it) {
				_cur_tsk_it = std::move(it);
				return _reset_cur_insert_it();
			}

			void _del_cur_tsk_and_it_inc() {
				auto it = _cur_tsk_it;
				return _set_cur_tsk_it(_tsks.erase(it));
			}

			void _reset_cur_insert_it() {
				if (_cur_tsk_it == _tsks.end()) {
					_cur_insert_it = _tsks.begin();
					return;
				}
				_cur_insert_it = _cur_tsk_it;
				++_cur_insert_it;
				return;
			}

			cont _orig_ct, _cur_tskr_ct;
			bin _cur_tskr_stk;
			std::vector<bin> _idle_tskr_stks;

			void _wake_cur_tsk() {
				_idle_tskr_stks.push_back(std::move(_cur_tskr_stk));
				_cur_tskr_stk = std::move(_cur_tsk_it->stk);
				_cur_tskr_ct.bind(&_tskr, this, _cur_tskr_stk);
				_cur_tsk_it->ct.restore();
			}

			static void _tskr(any_word p) {
				auto &cp = *p.to<co_pool *>();
				while (cp._cur_tsk_it != cp._tsks.end()) {
					if (cp._cur_tsk_it->is_sleeping()) {
						if (cp._cur_tsk_it->is_cw) {
							if (cp._cur_tsk_it->is_cv_notified->exchange(false)) {
								cp._cur_tsk_it->is_cw = false;
								cp._cur_tsk_it->cv_st = std::cv_status::no_timeout;
								cp._wake_cur_tsk();
							}
							if (cp._cur_tsk_it->wake_ti <= _cur_ti()) {
								cp._cur_tsk_it->cv_st = std::cv_status::timeout;
								cp._wake_cur_tsk();
							}
							cp._cur_tsk_it_inc();
							continue;
						}
						if (cp._cur_tsk_it->wake_ti <= _cur_ti()) {
							cp._wake_cur_tsk();
						}
						cp._cur_tsk_it_inc();
						continue;
					}

					if (!cp._cur_tsk_it->is_rept) {
						cp._cur_tsk_it->fn();
						cp._del_cur_tsk_and_it_inc();
						continue;
					}

					if (cp._cur_tsk_it->rept_end_ti <= _cur_ti()) {
						cp._del_cur_tsk_and_it_inc();
						continue;
					}

					cp._cur_tsk_it->fn();
					cp._cur_tsk_it_inc();
				}
				cp._orig_ct.restore();
			}

			void _new_tskr_co(cont &cur_cont_saver) {
				if (!_cur_tskr_stk) {
					if (_idle_tskr_stks.empty()) {
						_cur_tskr_stk.reset(8 * 1024 * 1024);
					} else {
						_cur_tskr_stk = std::move(_idle_tskr_stks.back());
						_idle_tskr_stks.pop_back();
					}
					_cur_tskr_ct.bind(&_tskr, this, _cur_tskr_stk);
				}
				_cur_tskr_ct.restore(cur_cont_saver);
			}

			static size_t _cur_ti() {
				return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock().now().time_since_epoch()).count();
			}

			bool _need_ll_sleep;
			size_t _ll_wake_ti;

			size_t _need_ll_cw;
			bool _ll_cv_changed;
			std::shared_ptr<scheduler::cond_var> _ll_cv;
			std::mutex _ll_cv_mtx;

			void _ll_cv_notify() {
				get_scheduler().lock(_ll_cv_mtx);
				_ll_cv_changed = true;
				if (_ll_cv) {
					auto ll_cv = _ll_cv;
					_ll_cv_mtx.unlock();
					ll_cv->notify();
					return;
				}
				_ll_cv_mtx.unlock();
			}

			class _scheduler : public scheduler {
				public:
					_scheduler() = default;

					_scheduler(co_pool &cp) : _cp(&cp) {}

					virtual ~_scheduler() = default;

					virtual void sleep(size_t timeout) {
						_cp->_cur_tsk_it->wake_ti = timeout ? co_pool::_cur_ti() + timeout : 0;

						_cp->_need_ll_sleep = true;
						if (timeout < _cp->_ll_wake_ti) {
							_cp->_ll_wake_ti = timeout;
						}

						_cp->_cur_tsk_it->stk = std::move(_cp->_cur_tskr_stk);
						auto it = _cp->_cur_tsk_it;
						_cp->_cur_tsk_it_inc();
						_cp->_new_tskr_co(it->ct);
					}

					class cond_var : public scheduler::cond_var {
						public:
							cond_var(co_pool &cp) : _cp(&cp), _n(std::make_shared<std::atomic<bool>>(false)) {}

							virtual void notify() {
								_n->store(true);
								_cp->_ll_cv_notify();
							}

						private:
							co_pool *_cp;
							std::shared_ptr<std::atomic<bool>> _n;
							friend _scheduler;
					};

					virtual std::shared_ptr<scheduler::cond_var> make_cond_var() {
						return std::static_pointer_cast<scheduler::cond_var>(std::make_shared<cond_var>(*_cp));
					}

					virtual std::cv_status cond_wait(std::shared_ptr<scheduler::cond_var> cv, typeless_lock_ref &lck, size_t timeout = nmax<size_t>()) {
						auto ncv = std::static_pointer_cast<cond_var>(cv);
						assert(ncv->_cp == _cp);
						_cp->_cur_tsk_it->is_cw = true;
						_cp->_cur_tsk_it->is_cv_notified = ncv->_n;

						++_cp->_need_ll_cw;

						lck.unlock();
						sleep(timeout);

						_cp->_cur_tsk_it->is_cw = false;
						--_cp->_need_ll_cw;

						lock(lck);

						return _cp->_cur_tsk_it->cv_st;
					}

				private:
					co_pool *_cp;
			} _sch;

			friend _scheduler;
	};
}

#endif
