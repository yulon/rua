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
#include <thread>
#include <chrono>
#include <mutex>
#include <cassert>

namespace tmd {
	class co_pool {
		public:
			co_pool(const co_pool &) = delete;
			co_pool& operator=(const co_pool &) = delete;
			co_pool(co_pool &&) = delete;
			co_pool& operator=(co_pool &&) = delete;

		private:
			class _task_s;

		public:
			typedef std::shared_ptr<_task_s> task_t;

			task_t add_task(const std::function<void()> &handler, int life_duration = -1) {
				auto task = std::make_shared<_task_s>();

				task->handler = handler;
				task->sleeping = nullptr;

				if (_in_work_td()) {
					task->del_time = life_duration < 0 ? 0 : _cur_time + life_duration;
					task->state = _task_s::state_t::added;

					_tasks.push_back(task);
					task->it = --_tasks.end();

				} else {
					task->del_time = life_duration < 0 ? 0 : _tick() + life_duration;
					task->state = _task_s::state_t::adding;

					_oth_td_op_mtx.lock();
					_pre_add_tasks.push_back(task);
					_oth_td_op_mtx.unlock();
				}

				return task;
			}

			task_t go(const std::function<void()> &handler) {
				return add_task(handler, 0);
			}

			void wait(size_t ms) {
				assert(in_task());

				(*_tasks_it)->sleeping.sleep_to = _cur_time + ms;
				_join_new_task_cor((*_tasks_it)->sleeping.con);
			}

			void wait(const std::function<bool()> &wake_cond) {
				assert(in_task());

				(*_tasks_it)->sleeping.wake_cond = wake_cond;
				_join_new_task_cor((*_tasks_it)->sleeping.con);
			}

			template <typename T>
			void lock(T &try_locker) {
				if (!try_locker.try_lock()) {
					wait([&try_locker]() {
						return try_locker.try_lock();
					});
				}
			}

			void del_task(task_t task) {
				if (_in_work_td()) {
					if (task->state == _task_s::state_t::added) {
						if (task->it == _tasks_it) {
							_tasks_it = _tasks.erase(_tasks_it);
							return;
						}

						_tasks.erase(task->it);

						if (task->sleeping) {
							for (auto it = _cors.begin(); it != _cors.end(); ++it) {
								if (task->sleeping.con.belong_to(*it)) {
									_cors.erase(it);
									break;
								}
							}
							task->sleeping = nullptr;
						}
					} else if (task->state == _task_s::state_t::adding) {
						// Just mark, main loop help us delete it.
						task->state = _task_s::state_t::deleted;
					}

				} else {
					_oth_td_op_mtx.lock();

					// 'task->state' cannot be checked here, may be accessed by tasks thread.
					for (auto it = _pre_add_tasks.begin(); it != _pre_add_tasks.end(); ++it) {
						if (task == *it) {
							_pre_add_tasks.erase(it);

							// It's safe here, unless you have other threads accessing at the same time.
							task->state = _task_s::state_t::deleted;

							_oth_td_op_mtx.unlock();
							return;
						}
					}

					_pre_del_tasks.push(task);

					_oth_td_op_mtx.unlock();
				}
			}

			bool has_task(task_t task) {
				// May be dirty read on other threads, but repeated calls to 'del_task()' do not cause an exception.
				return task->state != _task_s::state_t::deleted;
			}

			void del_this_task() {
				assert(in_task());

				_tasks_it = _tasks.erase(_tasks_it);
			}

			void handle_tasks(const std::function<void()> &yield = std::this_thread::yield) {
				if (_in_task_cors && !support_co_for_this_thread()) {
					return;
				}

				_work_tid = std::this_thread::get_id();

				for (;;) {
					_cur_time = _tick();

					if (_oth_td_op_mtx.try_lock()) {
						while (_pre_del_tasks.size()) {
							del_task(_pre_del_tasks.top());
							_pre_del_tasks.pop();
						}

						while (_pre_add_tasks.size()) {
							auto &task = _pre_add_tasks.front();
							if (task->state == _task_s::state_t::adding) {
								task->state = _task_s::state_t::added;
								_tasks.push_back(task);
							}
							_pre_add_tasks.pop_front();
						}

						_oth_td_op_mtx.unlock();
					}

					_tasks_it = _tasks.begin();

					_in_task_cors = true;
					_join_new_task_cor(_main_con);
					_in_task_cors = false;

					if (yield) {
						yield();
					}
				}
			}

			bool in_task() {
				return _in_work_td() && _in_task_cors;
			}

		private:
			std::thread::id _work_tid = std::this_thread::get_id();
			bool _in_work_td() {
				return std::this_thread::get_id() == _work_tid;
			}

			bool _in_task_cors = false;

			coroutine::continuation _main_con;
			std::list<coroutine> _cors;
			std::stack<coroutine::continuation> _idle_cor_cons;

			typedef std::list<std::shared_ptr<_task_s>> _task_list_t;

			struct _task_s {
				std::function<void()> handler;
				size_t del_time;

				struct {
					size_t sleep_to;
					std::function<bool()> wake_cond;
					coroutine::continuation con;

					operator bool() {
						return con;
					}

					void operator=(std::nullptr_t) {
						con = nullptr;
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

			std::list<task_t> _pre_add_tasks;
			std::stack<task_t> _pre_del_tasks;
			std::mutex _oth_td_op_mtx;

			void _join_new_task_cor(coroutine::continuation &ccr) {
				if (_idle_cor_cons.empty()) {
					_cors.emplace_back([this]() {
						for (;;) {
							while (_tasks_it != _tasks.end()) {
								if ((*_tasks_it)->sleeping) {
									if ((*_tasks_it)->sleeping.wake_cond) {
										if (!(*_tasks_it)->sleeping.wake_cond()) {
											++_tasks_it;
											continue;
										}
									} else if (_cur_time < (*_tasks_it)->sleeping.sleep_to) {
										++_tasks_it;
										continue;
									}
									auto con = (*_tasks_it)->sleeping.con;
									(*_tasks_it)->sleeping = nullptr;
									_idle_cor_cons.emplace();
									con.join(_idle_cor_cons.top());

								} else if ((*_tasks_it)->del_time > 0 && (*_tasks_it)->del_time >= _cur_time) {
									_tasks_it = _tasks.erase(_tasks_it);
								} else {
									(*_tasks_it)->handler();
									++_tasks_it;
								}
							}
							_idle_cor_cons.emplace();
							_main_con.join(_idle_cor_cons.top());
						}
					});
					_cors.back().execute(ccr);
				} else {
					auto con = _idle_cor_cons.top();
					_idle_cor_cons.pop();
					con.join(ccr);
				}
			}
	};
}

#endif
