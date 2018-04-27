#ifndef _RUA_CP_CORO_POOL_HPP
#define _RUA_CP_CORO_POOL_HPP

#include "coro.hpp"
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
		template <typename Coro>
		class base_coro_pool {
			public:
				base_coro_pool(size_t coro_stack_size = Coro::default_stack_size) :
					_exit_on_empty(true),
					_running(false),
					_co_stk_sz(coro_stack_size),
					_pre_add_tasks_sz(0),
					_notify_all(false)
				{
					bind_this_thread();
					_tasks_it = _tasks.end();
				}

				base_coro_pool(const base_coro_pool<Coro> &) = delete;
				base_coro_pool &operator=(const base_coro_pool<Coro> &) = delete;
				base_coro_pool(base_coro_pool<Coro> &&) = delete;
				base_coro_pool &operator=(base_coro_pool<Coro> &&) = delete;

			private:
				struct _task_info_t;

			public:
				using task = std::shared_ptr<_task_info_t>;

				class dur_t : public std::chrono::milliseconds {
					public:
						static constexpr size_t forever = static_cast<size_t>(-1);
						static constexpr size_t disposable = 0;

						dur_t() = default;

						constexpr dur_t(size_t ms) : std::chrono::milliseconds(
							ms == static_cast<size_t>(-1) ?
							std::chrono::milliseconds::max() :
							(ms == 0 ? std::chrono::milliseconds::min() : std::chrono::milliseconds(ms))
						) {}
				};

				task add(task pos, std::function<void()> handler, dur_t timeout = dur_t::forever) {
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

				task add(std::function<void()> handler, dur_t timeout = dur_t::forever) {
					return add(current(), std::move(handler), timeout);
				}

				task go(std::function<void()> handler) {
					return add(std::move(handler), dur_t::disposable);
				}

				task add_front(std::function<void()> handler, dur_t timeout = dur_t::forever) {
					return add(nullptr, std::move(handler), timeout);
				}

				task add_back(std::function<void()> handler, dur_t timeout = dur_t::forever) {
					return add(back(), std::move(handler), timeout);
				}

				void sleep(task tsk, dur_t timeout) {
					assert(this_thread_is_binded() && has(tsk));

					tsk->sleep_info.wake_time = _cur_time + timeout;

					_sleep(tsk);
				}

				void sleep(dur_t timeout) {
					sleep(current(), timeout);
				}

				void yield(task tsk) {
					sleep(tsk, 0);
				}

				void yield() {
					yield(current());
				}

				void cond_wait(task tsk, std::mutex &lock, dur_t timeout = dur_t::forever) {
					assert(this_thread_is_binded() && has(tsk));

					tsk->sleep_info.cv_notified = false;

					lock.unlock();

					tsk->sleep_info.wake_time = _cur_time + timeout;
					tsk->sleep_info.cv_lock = &lock;

					_sleep(tsk);
				}

				void cond_wait(std::mutex &lock, dur_t timeout = dur_t::forever) {
					cond_wait(current(), lock, timeout);
				}

				void cond_wait(task tsk, std::mutex &lock, std::function<bool()> pred, dur_t timeout = dur_t::forever) {
					assert(this_thread_is_binded() && has(tsk));

					auto wake_time = _cur_time + timeout;

					while (!pred()) {
						cond_wait(tsk, lock, std::chrono::duration_cast<std::chrono::milliseconds>(wake_time - _cur_time).count());
						if (_is_expiration(wake_time)) {
							return;
						}
					}
				}

				void cond_wait(std::mutex &lock, std::function<bool()> pred, dur_t timeout = dur_t::forever) {
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

				void reset_dol(task tsk, dur_t timeout) {
					assert(this_thread_is_binded() && has(tsk));

					tsk->timeout = timeout;
				}

				void reset_dol(dur_t timeout) {
					reset_dol(current(), timeout);
				}

				void erase(task tsk) {
					assert(this_thread_is_binded() && has(tsk));

					if (tsk->state == _task_info_t::state_t::added) {
						if (tsk.get() == _tasks_it->get()) {
							reset_dol(tsk, dur_t::disposable);
							return;
						}

						tsk->state = _task_info_t::state_t::deleted;

						if (tsk->sleeping && tsk->sleep_info.co) {
							tsk->sleep_info.co.reset();
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
						_main_co = Coro::from_this_thread();
						_join_new_task_co();
					}
				}

				size_t size() const {
					assert(this_thread_is_binded());

					return _tasks.size() + _pre_add_tasks_sz.load();
				}

				size_t empty() const {
					return !size();
				}

				/*size_t coro_total() const {
					assert(this_thread_is_binded());

					return _cos.size();
				}*/

				void exit() {
					assert(this_thread_is_binded());

					_life = false;
				}

				void exit_on_empty(bool toggle = true) {
					assert(this_thread_is_binded());

					_exit_on_empty = toggle;
				}

				class cond_var_c : public cp::cond_var_c {
					public:
						cond_var_c(base_coro_pool<Coro> &cp) : _cp(cp), _tsk(cp.current()) {
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
						base_coro_pool<Coro> &_cp;
						typename base_coro_pool<Coro>::task _tsk;
				};

				using cond_var = obj<cond_var_c>;

				class scheduler_c : public cp::scheduler_c {
					public:
						scheduler_c(base_coro_pool<Coro> &cp) : _cp(cp) {}

						virtual ~scheduler_c() = default;

						virtual bool is_available() const {
							return _cp.this_caller_in_task();
						}

						virtual void yield() const {
							_cp.yield();
						}

						virtual cp::cond_var make_cond_var() const {
							return cond_var(_cp);
						}

					private:
						base_coro_pool<Coro> &_cp;
				};

				using scheduler = obj<scheduler_c>;

				scheduler get_scheduler() {
					return scheduler(*this);
				}

			private:
				std::thread::id _work_tid;

				bool _life, _exit_on_empty, _running;

				size_t _co_stk_sz;

				Coro _main_co;
				std::stack<Coro> _idle_cos;

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

						_time_point operator+(const dur_t &dur) {
							return
								dur == dur_t(dur_t::forever) ?
								_time_point::max() :
								static_cast<const std::chrono::steady_clock::time_point &>(*this) + dur
							;
						}
				};

				struct _task_info_t {
					std::function<void()> handler;
					_time_point start_time;
					dur_t timeout;

					struct {
						_time_point wake_time;
						std::mutex *cv_lock;
						std::atomic<bool> cv_notified;
						Coro co;
					} sleep_info;

					bool sleeping;

					typename _task_list_t::iterator it;

					enum class state_t : uint8_t {
						deleted,
						added,
						adding
					};

					std::atomic<state_t> state;
				};

				_task_list_t _tasks;
				typename _task_list_t::iterator _tasks_it;

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
						(*_tasks_it++)->sleep_info.co = Coro::from_this_thread();
						_join_new_task_co();
					}
				}

				void _wake() {
					(*_tasks_it)->sleeping = false;

					if ((*_tasks_it)->sleep_info.co) {
						_running = true;
						_idle_cos.emplace(Coro::from_this_thread());
						(*_tasks_it)->sleep_info.co.join_and_detach();
					}
				}

				void _join_new_task_co() {
					if (_idle_cos.empty()) {
						Coro([this]() {
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
								_idle_cos.emplace(Coro::from_this_thread());
								_main_co.join_and_detach();
							}
						}, _co_stk_sz).join_and_detach();
					} else {
						auto co = std::move(_idle_cos.top());
						_idle_cos.pop();
						co.join_and_detach();
					}
				}
		};

		using coro_pool = base_coro_pool<coro>;
	}
}

#endif
