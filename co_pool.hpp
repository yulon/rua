#ifndef _TMD_CO_POOL_HPP
#define _TMD_CO_POOL_HPP

#include "co.hpp"

#if defined(_TMD_UNIX_)
	#include <time.h>
#endif

#include <functional>
#include <string>
#include <list>
#include <stack>
#include <queue>
#include <thread>
#include <chrono>
#include <mutex>
#include <cassert>

namespace tmd {
	class co_pool {
		public:
			co_pool(size_t coro_stack_size = coro::default_stack_size) {
				_co_stk_sz = coro_stack_size;
			}

			co_pool(const co_pool &) = delete;
			co_pool& operator=(const co_pool &) = delete;
			co_pool(co_pool &&) = delete;
			co_pool& operator=(co_pool &&) = delete;

		private:
			struct _task_info_t;

		public:
			typedef std::shared_ptr<_task_info_t> task;

			static constexpr int duration_always = -1;
			static constexpr int duration_disposable = 0;

			task add_task(const std::function<void()> &handler, int duration_of_life = duration_always) {
				auto tsk = std::make_shared<_task_info_t>();

				tsk->handler = handler;

				if (_in_work_td()) {
					tsk->del_time = duration_of_life < 0 ? 0 : _cur_time + duration_of_life;
					tsk->state = _task_info_t::state_t::added;

					_tasks.emplace_back(tsk);
					tsk->it = --_tasks.end();

				} else {
					tsk->del_time = static_cast<size_t>(duration_of_life);
					tsk->state = _task_info_t::state_t::adding;

					_oth_td_op_mtx.lock();
					_pre_add_tasks.emplace(tsk);
					_oth_td_op_mtx.unlock();
				}

				return tsk;
			}

			void go(const std::function<void()> &handler) {
				add_task(handler, duration_disposable);
			}

			void wait(size_t ms) {
				(*_tasks_it)->sleeping.sleep_to = _cur_time + ms;
				_sleep();
			}

			void wait(const std::function<bool()> &wake_cond) {
				(*_tasks_it)->sleeping.wake_cond = wake_cond;
				_sleep();
			}

			template <typename T>
			void lock(T &try_locker) {
				if (!try_locker.try_lock()) {
					wait([&try_locker]() {
						return try_locker.try_lock();
					});
				}
			}

			void del_task(task tsk) {
				assert(_in_work_td());

				if (tsk->state == _task_info_t::state_t::added) {
					if (tsk.get() == (*_tasks_it).get()) {
						reset_task_dol(tsk, duration_disposable);
						return;
					}

					tsk->state = _task_info_t::state_t::deleted;

					if (tsk->sleeping) {
						for (auto it = _cos.begin(); it != _cos.end(); ++it) {
							if (tsk->sleeping.ct.native_resource_handle() == it->native_resource_handle()) {
								_cos.erase(it);
								break;
							}
						}
					}

					_tasks.erase(tsk->it);
				}
			}

			bool has_task(task tsk) {
				assert(_in_work_td());

				return tsk->state != _task_info_t::state_t::deleted;
			}

			void reset_task_dol(task tsk, int duration_of_life) {
				assert(_in_work_td());

				tsk->del_time = duration_of_life < 0 ? 0 : _cur_time + duration_of_life;
			}

			task get_this_task() {
				assert(in_task());

				return *_tasks_it;
			}

			void handle_tasks(const std::function<void()> &yield = std::this_thread::yield) {
				assert(!in_task());

				_work_tid = std::this_thread::get_id();

				for (;;) {
					_cur_time = _tick();

					if (_oth_td_op_mtx.try_lock()) {
						while (_pre_add_tasks.size()) {
							auto &tsk = _pre_add_tasks.front();
							if (tsk->state == _task_info_t::state_t::adding) {
								tsk->state = _task_info_t::state_t::added;

								int duration_of_life = static_cast<int>(tsk->del_time);
								tsk->del_time = duration_of_life < 0 ? 0 : _cur_time + duration_of_life;

								_tasks.emplace_back(std::move(tsk));
								_tasks.back()->it = --_tasks.end();
							}
							_pre_add_tasks.pop();
						}
						_oth_td_op_mtx.unlock();
					}

					_tasks_it = _tasks.begin();

					_join_new_task_cor(_main_ct);

					if (yield) {
						yield();
					}
				}
			}

			bool in_task() {
				return _in_work_td() && _in_task;
			}

			size_t get_coro_total() {
				return _cos.size();
			}

		private:
			std::thread::id _work_tid = std::this_thread::get_id();
			bool _in_work_td() {
				return std::this_thread::get_id() == _work_tid;
			}

			bool _in_task = false;

			size_t _co_stk_sz;
			std::list<coro> _cos;

			cont _main_ct;
			std::stack<cont> _idle_co_cts;

			typedef std::list<std::shared_ptr<_task_info_t>> _task_list_t;

			struct _task_info_t {
				std::function<void()> handler;
				size_t del_time;

				struct {
					size_t sleep_to;
					std::function<bool()> wake_cond;
					cont ct;

					operator bool() {
						return ct;
					}
				} sleeping;

				_task_list_t::iterator it;

				enum class state_t : uint8_t {
					deleted,
					added,
					adding
				} state;
			};

			_task_list_t _tasks;
			_task_list_t::iterator _tasks_it;

			size_t _cur_time;

			static size_t _tick() {
				#if defined(_WIN32)
					return GetTickCount();
				#elif defined(_TMD_UNIX_)
					timespec ts;
					clock_gettime(CLOCK_MONOTONIC, &ts);
					return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
				#endif
			}

			std::queue<task> _pre_add_tasks;
			std::mutex _oth_td_op_mtx;

			void _sleep() {
				assert(in_task());

				_in_task = false;
				_join_new_task_cor((*_tasks_it++)->sleeping.ct);
			}

			void _wake() {
				_in_task = true;
				_idle_co_cts.emplace();
				(*_tasks_it)->sleeping.ct.join(_idle_co_cts.top());
			}

			void _join_new_task_cor(cont &ccr) {
				if (_idle_co_cts.empty()) {
					_cos.emplace_back([this]() {
						for (;;) {
							while (_tasks_it != _tasks.end()) {
								auto &tsk = *_tasks_it;

								if (tsk->sleeping) {
									if (tsk->sleeping.wake_cond) {
										if (tsk->sleeping.wake_cond()) {
											tsk->sleeping.wake_cond = nullptr;
											_wake();
											continue;
										}
									} else if (_cur_time >= tsk->sleeping.sleep_to) {
										_wake();
										continue;
									}
									++_tasks_it;
									continue;
								}

								_in_task = true;
								tsk->handler();
								_in_task = false;

								if (tsk->del_time > 0 && _cur_time >= tsk->del_time) {
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

#endif
