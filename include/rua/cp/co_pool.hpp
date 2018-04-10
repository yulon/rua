#ifndef _RUA_CP_CO_POOL_HPP
#define _RUA_CP_CO_POOL_HPP

#include "co.hpp"
#include "sched.hpp"

#include <functional>
#include <string>
#include <list>
#include <stack>
#include <deque>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <limits>
#include <cassert>

#include "../disable_msvc_sh1t.h"

namespace rua {
	namespace cp {
		class co_pool {
			public:
				co_pool(size_t coro_stack_size = coro::default_stack_size) :
					_exit_on_empty(true),
					_running(false),
					_co_stk_sz(coro_stack_size),
					_pre_add_tasks_sz(0),
					_notify_all(false)
				{
					bind_this_thread();
					_tasks_it = _tasks.end();
				}

				co_pool(const co_pool &) = delete;
				co_pool &operator=(const co_pool &) = delete;
				co_pool(co_pool &&) = delete;
				co_pool &operator=(co_pool &&) = delete;

			private:
				struct _task_info_t;

			public:
				using task = std::shared_ptr<_task_info_t>;

				class duration : public std::chrono::milliseconds {
					public:
						static constexpr size_t forever = static_cast<size_t>(-1);
						static constexpr size_t disposable = 0;

						duration() = default;

						constexpr duration(size_t ms) : std::chrono::milliseconds(
							ms == static_cast<size_t>(-1) ?
							std::chrono::milliseconds::max() :
							(ms == 0 ? std::chrono::milliseconds::min() : std::chrono::milliseconds(ms))
						) {}
				};

				task add(task pos, std::function<void()> handler, duration timeout = duration::forever) {
					auto tsk = std::make_shared<_task_info_t>();

					tsk->handler = std::move(handler);
					tsk->timeout = timeout;
					tsk->sleeping = false;

					if (this_thread_is_binded()) {
						tsk->start_time = _cur_time;
						tsk->state = _task_info_t::state_t::added;

						if (has(pos)) {
							auto pos_it = pos->it;
							_tasks.insert(++pos_it, tsk);
							(*(--pos_it))->it = pos_it;
						} else {
							_tasks.emplace_front(tsk);
							_tasks.front()->it = _tasks.begin();
						}

					} else {
						tsk->state = _task_info_t::state_t::adding;

						_oth_td_op_mtx.lock();
						_pre_add_tasks.emplace_front(_pre_task_info_t{tsk, std::move(pos)});
						++_pre_add_tasks_sz;
						_oth_td_op_mtx.unlock();
					}

					return tsk;
				}

				task add(std::function<void()> handler, duration timeout = duration::forever) {
					return add(current(), std::move(handler), timeout);
				}

				task go(std::function<void()> handler) {
					return add(std::move(handler), duration::disposable);
				}

				task add_front(std::function<void()> handler, duration timeout = duration::forever) {
					return add(nullptr, std::move(handler), timeout);
				}

				task add_back(std::function<void()> handler, duration timeout = duration::forever) {
					return add(back(), std::move(handler), timeout);
				}

				void sleep(task tsk, duration timeout) {
					assert(this_thread_is_binded() && has(tsk));

					tsk->sleep_info.wake_time = _cur_time + timeout;

					_sleep(tsk);
				}

				void sleep(duration timeout) {
					sleep(current(), timeout);
				}

				void yield(task tsk) {
					sleep(tsk, 0);
				}

				void yield() {
					yield(current());
				}

				void cond_wait(task tsk, std::mutex &lock, duration timeout = duration::forever) {
					assert(this_thread_is_binded() && has(tsk));

					tsk->sleep_info.cv_notified = false;

					lock.unlock();

					tsk->sleep_info.wake_time = _cur_time + timeout;
					tsk->sleep_info.cv_lock = &lock;

					_sleep(tsk);
				}

				void cond_wait(std::mutex &lock, duration timeout = duration::forever) {
					cond_wait(current(), lock, timeout);
				}

				void cond_wait(task tsk, std::mutex &lock, std::function<bool()> pred, duration timeout = duration::forever) {
					assert(this_thread_is_binded() && has(tsk));

					auto wake_time = _cur_time + timeout;

					while (!pred()) {
						cond_wait(tsk, lock, std::chrono::duration_cast<std::chrono::milliseconds>(wake_time - _cur_time).count());
						if (_is_expiration(wake_time)) {
							return;
						}
					}
				}

				void cond_wait(std::mutex &lock, std::function<bool()> pred, duration timeout = duration::forever) {
					cond_wait(current(), lock, std::move(pred), timeout);
				}

				void notify(task tsk) {
					assert(has(tsk));

					tsk->sleep_info.cv_notified = true;
				}

				void notify() {
					notify(current());
				}

				void notify_all() {
					if (this_thread_is_binded()) {
						for (auto &tsk : _tasks) {
							if (tsk->sleeping && tsk->sleep_info.cv_lock) {
								notify(tsk);
							}
						}
					} else {
						_notify_all = true;
					}
				}

				void reset_dol(task tsk, duration duration) {
					assert(this_thread_is_binded() && has(tsk));

					tsk->timeout = duration;
				}

				void reset_dol(duration duration) {
					reset_dol(current(), duration);
				}

				void erase(task tsk) {
					assert(this_thread_is_binded() && has(tsk));

					if (tsk->state == _task_info_t::state_t::added) {
						if (tsk.get() == _tasks_it->get()) {
							reset_dol(tsk, duration::disposable);
							return;
						}

						tsk->state = _task_info_t::state_t::deleted;

						if (tsk->sleeping && tsk->sleep_info.ct) {
							for (auto it = _cos.begin(); it != _cos.end(); ++it) {
								if (tsk->sleep_info.ct.native_resource_handle() == it->native_resource_handle()) {
									_cos.erase(it);
									break;
								}
							}
						}

						_tasks.erase(tsk->it);

					} else {
						tsk->state = _task_info_t::state_t::deleted;
					}
				}

				void erase() {
					erase(current());
				}

				void erase_all(bool erase_pre_add_tasks = false) {
					for (auto it = _tasks.begin(); it != _tasks.end(); ++it) {
						erase(*it);
					}
					if (erase_pre_add_tasks && _pre_add_tasks_sz) {
						_oth_td_op_mtx.lock();
						while (_pre_add_tasks.size()) {
							auto &pt = _pre_add_tasks.front();
							if (pt.tsk->state == _task_info_t::state_t::adding) {
								pt.tsk->state = _task_info_t::state_t::deleted;
							}
							_pre_add_tasks.pop_front();
							--_pre_add_tasks_sz;
						}
						_oth_td_op_mtx.unlock();
					}
				}

				bool has(task tsk) const {
					return tsk && tsk->state != _task_info_t::state_t::deleted;
				}

				task current() const {
					return this_thread_is_binded() && _tasks_it != _tasks.end() ? *_tasks_it : nullptr;
				}

				task running() const {
					assert(this_thread_is_binded());

					return _running ? *_tasks_it : nullptr;
				}

				bool is_running() const {
					assert(this_thread_is_binded());

					return _running;
				}

				bool this_caller_in_task() const {
					return this_thread_is_binded() && _running;
				}

				task front() const {
					assert(this_thread_is_binded());

					return _tasks.size() ? _tasks.front() : nullptr;
				}

				task back() const {
					assert(this_thread_is_binded());

					return _tasks.size() ? _tasks.back() : nullptr;
				}

				void bind_this_thread() {
					_work_tid = std::this_thread::get_id();
				}

				std::thread::id binded_thread_id() const {
					return _work_tid;
				}

				bool this_thread_is_binded() const {
					return std::this_thread::get_id() == _work_tid;
				}

				void handle() {
					assert(this_thread_is_binded() && !is_running());

					if (_exit_on_empty ? size() : true) {
						_life = true;
						if (_notify_all.exchange(false)) {
							notify_all();
						}
						_join_new_task_cor(_main_ct);
					}
				}

				size_t size() const {
					assert(this_thread_is_binded());

					return _tasks.size() + _pre_add_tasks_sz.load();
				}

				size_t empty() const {
					return !size();
				}

				size_t coro_total() const {
					assert(this_thread_is_binded());

					return _cos.size();
				}

				void exit() {
					assert(this_thread_is_binded());

					_life = false;
				}

				void exit_on_empty(bool toggle = true) {
					assert(this_thread_is_binded());

					_exit_on_empty = toggle;
				}

				class cond_var_c : public rua::cond_var_c {
					public:
						cond_var_c(co_pool &cp) : _cp(cp), _tsk(cp.current()) {
							assert(cp.this_caller_in_task());
						}

						virtual ~cond_var_c() = default;

						virtual void cond_wait(std::mutex &lock, std::function<bool()> pred) {
							assert(_cp.this_caller_in_task() && _tsk.get() == _cp.current().get());

							_cp.cond_wait(_tsk, lock, pred);
						}

						virtual void notify() {
							_cp.notify(_tsk);
						}

					private:
						co_pool &_cp;
						co_pool::task _tsk;
				};

				using cond_var = obj<cond_var_c>;

				class scheduler_c : public rua::scheduler_c {
					public:
						scheduler_c(co_pool &cp) : _cp(cp) {}

						virtual ~scheduler_c() = default;

						virtual bool is_available() const {
							return _cp.this_caller_in_task();
						}

						virtual void yield() const {
							_cp.yield();
						}

						virtual rua::cond_var make_cond_var() const {
							return cond_var(_cp);
						}

					private:
						co_pool &_cp;
				};

				using scheduler = obj<scheduler_c>;

				scheduler get_scheduler() {
					return scheduler(*this);
				}

			private:
				std::thread::id _work_tid;

				bool _life, _exit_on_empty, _running;

				size_t _co_stk_sz;
				std::list<coro> _cos;

				cont _main_ct;
				std::stack<cont> _idle_co_cts;

				using _task_list_t = std::list<task>;

				class _time_point : public std::chrono::steady_clock::time_point {
					public:
						_time_point() = default;

						_time_point(const std::chrono::steady_clock::time_point &std_tp) :
							std::chrono::steady_clock::time_point(std_tp)
						{}

						_time_point(std::chrono::steady_clock::time_point &&std_tp) :
							std::chrono::steady_clock::time_point(std::move(std_tp))
						{}

						_time_point operator+(const co_pool::duration &dur) {
							return
								dur == co_pool::duration(co_pool::duration::forever) ?
								_time_point::max() :
								static_cast<const std::chrono::steady_clock::time_point &>(*this) + dur
							;
						}
				};

				struct _task_info_t {
					std::function<void()> handler;
					_time_point start_time;
					duration timeout;

					struct {
						_time_point wake_time;
						std::mutex *cv_lock;
						std::atomic<bool> cv_notified;
						cont ct;
					} sleep_info;

					bool sleeping;

					_task_list_t::iterator it;

					enum class state_t : uint8_t {
						deleted,
						added,
						adding
					};

					std::atomic<state_t> state;
				};

				_task_list_t _tasks;
				_task_list_t::iterator _tasks_it;

				_time_point _cur_time;

				bool _is_expiration(_time_point time) {
					return time != _time_point::max() && _cur_time >= time;
				}

				std::mutex _oth_td_op_mtx;

				struct _pre_task_info_t {
					task tsk, pos;
				};

				std::deque<_pre_task_info_t> _pre_add_tasks;
				std::atomic<size_t> _pre_add_tasks_sz;

				std::atomic<bool> _notify_all;
				bool _notified_all;

				void _sleep(task &tsk) {
					tsk->sleeping = true;

					if (is_running() && tsk.get() == _tasks_it->get()) {
						tsk.reset();
						_running = false;
						_join_new_task_cor((*_tasks_it++)->sleep_info.ct);
					}
				}

				void _wake() {
					(*_tasks_it)->sleeping = false;

					if ((*_tasks_it)->sleep_info.ct) {
						_running = true;
						_idle_co_cts.emplace();
						(*_tasks_it)->sleep_info.ct.join(_idle_co_cts.top());
					}
				}

				void _join_new_task_cor(cont &ccr) {
					if (_idle_co_cts.empty()) {
						_cos.emplace_back([this]() {
							for (;;) {
								while (_life && (_exit_on_empty ? size() : true)) {
									if (_tasks_it == _tasks.end()) {
										_cur_time = std::chrono::steady_clock::now();

										if (_pre_add_tasks_sz && _oth_td_op_mtx.try_lock()) {
											while (_pre_add_tasks.size()) {
												auto &pt = _pre_add_tasks.front();
												if (pt.tsk->state == _task_info_t::state_t::adding) {
													pt.tsk->state = _task_info_t::state_t::added;

													pt.tsk->start_time = _cur_time;

													if (has(pt.pos)) {
														if (pt.pos->state == _task_info_t::state_t::adding) {
															_pre_add_tasks.emplace_back(std::move(pt));
															_pre_add_tasks.pop_front();
															continue;
														}
														auto pos_it = pt.pos->it;
														_tasks.insert(++pos_it, std::move(pt.tsk));
														(*(--pos_it))->it = pos_it;
													} else {
														_tasks.emplace_front(std::move(pt.tsk));
														_tasks.front()->it = _tasks.begin();
													}
												}
												_pre_add_tasks.pop_front();
												--_pre_add_tasks_sz;
											}
											_oth_td_op_mtx.unlock();
										}

										_tasks_it = _tasks.begin();
									}

									auto &tsk = *_tasks_it;

									if (tsk->sleeping) {
										if (_is_expiration(tsk->sleep_info.wake_time)) {
											if (tsk->sleep_info.cv_lock) {
												tsk->sleep_info.cv_lock = nullptr;
											}
											_wake();
											continue;
										}

										if (
											tsk->sleep_info.cv_lock &&
											tsk->sleep_info.cv_notified &&
											tsk->sleep_info.cv_lock->try_lock()
										) {
											tsk->sleep_info.cv_lock = nullptr;
											_wake();
											continue;
										}

										++_tasks_it;
										continue;
									}

									_running = true;
									tsk->handler();
									_running = false;

									if (_is_expiration(tsk->start_time + tsk->timeout)) {
										tsk->state = _task_info_t::state_t::deleted;
										_tasks_it = _tasks.erase(_tasks_it);
									} else {
										++_tasks_it;
									}
								}
								_idle_co_cts.emplace();
								_main_ct.join(_idle_co_cts.top());
							}
						}, _co_stk_sz);
						_cos.back().join(ccr);
					} else {
						auto ct = std::move(_idle_co_cts.top());
						_idle_co_cts.pop();
						ct.join(ccr);
					}
				}
		};
	}
}

#endif
