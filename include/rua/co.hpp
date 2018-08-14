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
				_tsks_ct.save();
			}

			/*enum class whence_t : uint8_t {
				all_back = 0,
				all_front,
				current_after,
				current_afters_back,
				current_before,
				current_befores_front
			};*/

			void add(std::function<void()> task,/* whence_t whence = whence_t::current_afters_back,*/ size_t timeout = 0) {
				_tsks.insert(_cur_insert_it, _tsk_ctx_t{
					std::move(task),
					timeout,
					nullptr,
					nullptr,
					0,
					0,
					false,
					nullptr,
					std::cv_status::no_timeout
				});
				/*switch (whence) {
					case whence_t::all_front:

						return;
					case whence_t::all_back:
						_tsks.push_back(std::move(ctx));
						return;
					case whence_t::current_after:
						_adding_afters.push_front(std::move(ctx));
						return;
					case whence_t::current_afters_back:
						_adding_afters.push_back(std::move(ctx));
						return;
					case whence_t::current_before:
						_adding_befores.push_back(std::move(ctx));
						return;
					case whence_t::current_befores_front:
						_adding_befores.push_front(std::move(ctx));
						return;
				}*/
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
					_new_tsks_handler_co(_orig_ct);

					//_init();
					//_tsks_ct.restore(_orig_ct);
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
				_new_tsks_handler_co(_orig_ct);

				//_init();
				//_tsks_ct.restore(_orig_ct);
			}

			scheduler &get_scheduler() {
				return _sch;
			}

		private:
			struct _tsk_ctx_t {
				std::function<void()> fn;
				size_t end_ti;

				cont ct;
				bin stack_data;
				size_t unused_stk_sz;

				bool is_sleeping() const {
					return stack_data;
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

			void _reset_cur_insert_it() {
				if (_cur_tsk_it == _tsks.end()) {
					_cur_insert_it = _tsks.begin();
					return;
				}
				_cur_insert_it = _cur_tsk_it;
				++_cur_insert_it;
				return;
			}

			/*void _add_addings() {
				while (_adding_befores.size()) {
					_tsks.insert(_cur_tsk_it, std::move(_adding_befores.front()));
					_adding_befores.pop_front();
				}

				if (_adding_afters.empty()) {
					return;
				}
				auto it = _cur_tsk_it;
				++it;
				do {
					_tsks.insert(it, std::move(_adding_afters.front()));
					_adding_afters.pop_front();
				} while (_adding_afters.size());
			}*/

			cont _orig_ct, _tsks_ct;
			bin _cur_stk;
			std::vector<bin> _idle_stks;

			static void _wake_tsk_x(_tsk_ctx_t *tsk_ptr, bin *tsks_stk_ptr) {
				tsks_stk_ptr->slice(tsks_stk_ptr->size() - tsk_ptr->stack_data.size()).copy(tsk_ptr->stack_data);
				tsk_ptr->stack_data.reset();
				tsk_ptr->ct.restore();
			}

			template <size_t PaddingSz>
			static void _wake_tsk(_tsk_ctx_t &tsk, bin &tsks_stk) {
				struct {
					uint8_t ctps[PaddingSz * 1024 * 1024];
					_tsk_ctx_t *tsk_ptr;
					bin *tsks_stk_ptr;
				} vars;

				vars.tsk_ptr = &tsk;
				vars.tsks_stk_ptr = &tsks_stk;

				_wake_tsk_x(vars.tsk_ptr, vars.tsks_stk_ptr);
			}

			void _wake_cur_tsk() {
				_idle_stks.push_back(std::move(_cur_stk));
				_cur_stk = std::move(_cur_tsk_it->stack_data);
				_tsks_ct.bind(&_tsks_handler, this, _cur_stk);
				_cur_tsk_it->ct.restore();

				/*auto tsks_stk_end = _cur_stk.base() + _cur_stk.size();

				auto using_sz = static_cast<size_t>(tsks_stk_end - any_ptr(&tsks_stk_end));
				if (using_sz >= _cur_tsk_it->stack_data.size()) {
					_wake_tsk<0>(*_cur_tsk_it, _cur_stk);
				}

				auto padding_sz = (_cur_tsk_it->stack_data.size() - using_sz) / (1024 * 1024) + 1;

				assert(padding_sz < 15);

				switch (padding_sz) {
					case 1:
						_wake_tsk<1>(*_cur_tsk_it, _cur_stk);
						break;
					case 2:
						_wake_tsk<2>(*_cur_tsk_it, _cur_stk);
						break;
					case 3:
						_wake_tsk<3>(*_cur_tsk_it, _cur_stk);
						break;
					case 4:
						_wake_tsk<4>(*_cur_tsk_it, _cur_stk);
						break;
					case 5:
						_wake_tsk<5>(*_cur_tsk_it, _cur_stk);
						break;
					case 6:
						_wake_tsk<6>(*_cur_tsk_it, _cur_stk);
						break;
					case 7:
						_wake_tsk<7>(*_cur_tsk_it, _cur_stk);
						break;
					case 8:
						_wake_tsk<8>(*_cur_tsk_it, _cur_stk);
						break;
					case 9:
						_wake_tsk<9>(*_cur_tsk_it, _cur_stk);
						break;
					case 10:
						_wake_tsk<10>(*_cur_tsk_it, _cur_stk);
						break;
					case 11:
						_wake_tsk<11>(*_cur_tsk_it, _cur_stk);
						break;
					case 12:
						_wake_tsk<12>(*_cur_tsk_it, _cur_stk);
						break;
					case 13:
						_wake_tsk<13>(*_cur_tsk_it, _cur_stk);
						break;
					case 14:
						_wake_tsk<14>(*_cur_tsk_it, _cur_stk);
				}*/
			}

			static void _tsks_handler(any_word p) {
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
					cp._cur_tsk_it->fn();
					if (cp._cur_tsk_it->end_ti <= _cur_ti()) {
						auto it = cp._cur_tsk_it;
						cp._set_cur_tsk_it(cp._tsks.erase(it));
						continue;
					}
					cp._cur_tsk_it_inc();
				}
				cp._orig_ct.restore();
			}

			void _init() {
				if (_cur_stk) {
					return;
				}
				_cur_stk.reset(16 * 1024 * 1024);
				_tsks_ct.bind(&_tsks_handler, this, _cur_stk);
			}

			void _new_tsks_handler_co(cont &cur_cont_saver) {
				if (!_cur_stk) {
					if (_idle_stks.empty()) {
						_cur_stk.reset(8 * 1024 * 1024);
					} else {
						_cur_stk = std::move(_idle_stks.back());
						_idle_stks.pop_back();
					}
					_tsks_ct.bind(&_tsks_handler, this, _cur_stk);
				}
				_tsks_ct.restore(cur_cont_saver);
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
						/*{
							if (!_cp->_cur_tsk_it->ct.save()) {
								return;
							}
						}

						{

							_cp->_cur_tsk_it->unused_stk_sz = any_ptr(_cp->_cur_tsk_it->ct.mctx.sp) - _cp->_cur_stk.base();
							auto using_stk_sz = _cp->_cur_stk.base() + _cp->_cur_stk.size() - any_ptr(_cp->_cur_tsk_it->ct.mctx.sp);
							assert(_cp->_cur_tsk_it->unused_stk_sz > 2 * 1024 * 1024);
							_cp->_cur_tsk_it->stack_data.reset(using_stk_sz);
							_cp->_cur_tsk_it->stack_data.copy(_cp->_cur_stk(_cp->_cur_tsk_it->unused_stk_sz));
							if (!_cp->_cur_tsk_it->stack_data.size()) {
								//return;
							}
							_cp->_tsks_ct.restore();
						}*/

						_cp->_cur_tsk_it->stack_data = std::move(_cp->_cur_stk);
						auto it = _cp->_cur_tsk_it;
						_cp->_cur_tsk_it_inc();
						_cp->_new_tsks_handler_co(it->ct);
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
