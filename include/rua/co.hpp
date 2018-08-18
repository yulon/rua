#ifndef _RUA_CO_HPP
#define _RUA_CO_HPP

#include "bin.hpp"
#include "cont.hpp"
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
	class co_pool {
		public:
			co_pool() : _sch(_scheduler(*this)) {
				_tskr_ct_base.save();
			}

			void add(std::function<void()> task, size_t timeout = 0) {
				auto tsk = std::make_shared<_tsk_t>();

				tsk->fn = std::move(task);

				_tsks.emplace(tsk);

				if (!timeout) {
					tsk->is_rept = false;
					return;
				}
				tsk->is_rept = true;

				if (timeout >= nmax<size_t>() - _now()) {
					tsk->rept_end_ti = nmax<size_t>();
					return;
				}
				tsk->rept_end_ti = _now() + timeout;
			}

			void enable_add_from_other_thread() {
				assert(false);
			}

			void add_from_other_thread(std::function<void()> task, size_t timeout = 0) {
				assert(false);
			}

			void step() {
				auto now = _now();

				if (_slps.size()) {
					_check_slps(now);
				}
				if (_cws.size()) {
					_check_cws(now);
				}
				if (_tsks.empty()) {
					return;
				}

				scheduler_guard sg(_sch);

				_handle_tsks();
			}

			void run() {
				if (_tsks.empty() && _slps.empty() && _cws.empty()) {
					return;
				}

				scheduler_guard sg(_sch);
				auto &ll_sch = sg.previous();
				auto ll_cv = ll_sch.make_cond_var();

				auto now = _now();

				if (_slps.size()) {
					_check_slps(now);
				}
				if (_cws.size()) {
					_check_cws(now);
				}

				for (;;) {
					if (_tsks.size()) {
						_handle_tsks();
					}

					now = _now();

					if (_slps.size()) {
						if (_cws.size()) {

							size_t wake_ti;
							if (_cws.begin()->second->wake_ti < _slps.begin()->second->wake_ti) {
								wake_ti = _cws.begin()->second->wake_ti;
							} else {
								wake_ti = _slps.begin()->second->wake_ti;
							}

							bool need_check_slps = true;
							for (;;) {
								if (_ll_cv_mtx.try_lock()) {
									if (_ll_cv_changed) {
										_ll_cv_changed = false;
										need_check_slps = false;
									} else {
										auto wake_ti = _cws.begin()->second->wake_ti;
										_ll_cv = ll_cv;
										ll_sch.cond_wait(ll_cv, _ll_cv_mtx, [this]() -> bool {
											return _ll_cv_changed;
										}, wake_ti - _now());
										_ll_cv_changed = false;
										_ll_cv.reset();
									}
									_ll_cv_mtx.unlock();
									now = _now();
									break;
								}
								ll_sch.yield();
								now = _now();
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

						auto wake_ti = _slps.begin()->second->wake_ti;
						if (wake_ti > now) {
							ll_sch.sleep(wake_ti - now);
						} else {
							ll_sch.yield();
						}

						now = _now();
						_check_slps(now);

						continue;

					} else if (_cws.size()) {

						ll_sch.lock(_ll_cv_mtx);

						if (_ll_cv_changed) {
							_ll_cv_changed = false;
						} else {
							auto wake_ti = _cws.begin()->second->wake_ti;
							_ll_cv = ll_cv;
							ll_sch.cond_wait(ll_cv, _ll_cv_mtx, [this]() -> bool {
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

			scheduler &get_scheduler() {
				return _sch;
			}

			size_t stack_count() const {
				auto c = _idle_tskr_stks.size();
				if (_cur_tskr_stk) {
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
			struct _tsk_t {
				std::function<void()> fn;

				bool is_rept;
				size_t rept_end_ti;

				cont ct;
				bin stk;

				size_t wake_ti;
				std::shared_ptr<std::atomic<bool>> is_cv_notified;
				std::cv_status cv_st;

				bool is_slept;
			};

			using _tsk_ptr_t = std::shared_ptr<_tsk_t>;

			std::queue<_tsk_ptr_t> _tsks;
			_tsk_ptr_t _cur_tsk;

			std::multimap<size_t, _tsk_ptr_t> _slps;
			std::multimap<size_t, _tsk_ptr_t> _cws;

			void _check_slps(size_t now) {
				for (auto it = _slps.begin(); it != _slps.end(); it = _slps.erase(it)) {
					if (now < it->second->wake_ti) {
						break;
					}
					_tsks.emplace(std::move(it->second));
				}
			}

			void _check_cws(size_t now) {
				for (auto it = _cws.begin(); it != _cws.end(); ) {
					if (it->second->is_cv_notified->exchange(false)) {
						it->second->cv_st = std::cv_status::no_timeout;
						_tsks.emplace(std::move(it->second));
						it = _cws.erase(it);
						continue;
					}
					if (now >= it->second->wake_ti) {
						it->second->cv_st = std::cv_status::timeout;
						_tsks.emplace(std::move(it->second));
						it = _cws.erase(it);
						continue;
					}
					++it;
				}
			}

			cont _orig_ct, _tskr_ct_base;

			bin _cur_tskr_stk;
			std::vector<bin> _idle_tskr_stks;

			void _restore_cur_tsk_stk() {
				_idle_tskr_stks.push_back(std::move(_cur_tskr_stk));
				_cur_tskr_stk = std::move(_cur_tsk->stk);
				_tskr_ct_base.bind(&_tskr, this, _cur_tskr_stk);
			}

			void _resume_cur_tsk() {
				_restore_cur_tsk_stk();
				_cur_tsk->ct.restore();
			}

			void _resume_cur_tsk(cont &ct) {
				_restore_cur_tsk_stk();
				_cur_tsk->ct.restore(ct);
			}

			static void _tskr(any_word p) {
				auto &cp = *p.to<co_pool *>();

				while (cp._tsks.size()) {
					cp._cur_tsk = std::move(cp._tsks.front());
					cp._tsks.pop();

					if (cp._cur_tsk->stk) {
						cp._resume_cur_tsk();
						continue;
					}

					if (!cp._cur_tsk->is_rept) {
						cp._cur_tsk->fn();
						continue;
					}

					for (;;) {
						if (cp._cur_tsk->rept_end_ti <= _now()) {
							break;
						}

						cp._cur_tsk->fn();

						if (!cp._cur_tsk->is_slept) {
							break;
						}
						cp._cur_tsk->is_slept = false;
					}
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
					_tskr_ct_base.bind(&_tskr, this, _cur_tskr_stk);
				}
				_tskr_ct_base.restore(cur_cont_saver);
			}

			void _handle_tsks() {
				if (_tsks.front()->stk) {
					_cur_tsk = std::move(_tsks.front());
					_tsks.pop();
					_resume_cur_tsk(_orig_ct);
					return;
				}
				_new_tskr_co(_orig_ct);
			}

			static size_t _now() {
				return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock().now().time_since_epoch()).count();
			}

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

					template <typename SlpMap>
					void _sleep(SlpMap &&slp_map, size_t timeout) {
						auto &tsk = *slp_map.emplace(
							timeout ? co_pool::_now() + timeout : 0,
							std::move(_cp->_cur_tsk)
						)->second;

						if (!timeout) {
							tsk.wake_ti = 0;
						} else if (timeout >= nmax<size_t>() - _now()) {
							tsk.wake_ti = nmax<size_t>();
						} else {
							tsk.wake_ti = co_pool::_now() + timeout;
						}

						tsk.is_slept = true;

						tsk.stk = std::move(_cp->_cur_tskr_stk);
						_cp->_new_tskr_co(tsk.ct);
					}

					virtual void sleep(size_t timeout) {
						_sleep(_cp->_slps, timeout);
					}

					virtual void yield() {
						sleep(0);
					}

					virtual void lock(typeless_lock_ref &lck) {
						while (!lck.try_lock()) {
							yield();
						}
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

						_cp->_cur_tsk->is_cv_notified = ncv->_n;

						lck.unlock();
						_sleep(_cp->_cws, timeout);

						lock(lck);
						return _cp->_cur_tsk->cv_st;
					}

				private:
					co_pool *_cp;
			} _sch;

			friend _scheduler;
	};
}

#endif
