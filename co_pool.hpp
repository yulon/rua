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

			uintptr_t add_task(const std::function<void()> &handler, int life_duration = -1) {
				auto t = new _task_t;

				t->handler = handler;
				t->sleeping = nullptr;

				if (_in_work_td()) {
					t->del_time = life_duration < 0 ? 0 : _cur_time + life_duration;

					_tasks.push_back(t);
					if (_tasks_it == _tasks.end()) {
						_tasks_it = --_tasks.end();
					}
				} else {
					t->del_time = life_duration < 0 ? 0 : _tick() + life_duration;

					_pre_add_tasks_mtx.lock();
					_pre_add_tasks.push(t);
					_pre_add_tasks_mtx.unlock();
				}

				return reinterpret_cast<uintptr_t>(t);
			}

			uintptr_t go(const std::function<void()> &handler) {
				return add_task(handler, 0);
			}

			bool del_task(uintptr_t id) {
				assert(_in_work_td());

				auto t = reinterpret_cast<_task_t *>(id);

				if (t == *_tasks_it) {
					_tasks.erase(_tasks_it--);
					delete t;
					return true;
				}

				for (auto it = _tasks.begin(); it != _tasks.end(); ++it) {
					if (t == *it) {
						_tasks.erase(it);
						if (t->sleeping) {
							for (auto it = _cors.begin(); it != _cors.end(); ++it) {
								if (t->sleeping->con.belong_to(*it)) {
									_cors.erase(it);
									break;
								}
							}
							delete t->sleeping;
						}
						delete t;
						return true;
					}
				}

				return false;
			}

			bool has_task(uintptr_t id) {
				assert(_in_work_td());

				auto t = reinterpret_cast<_task_t *>(id);

				if (t == *_tasks_it) {
					return true;
				}

				for (auto it = _tasks.begin(); it != _tasks.end(); ++it) {
					if (t == *it) {
						return true;
					}
				}

				return false;
			}

			void wait(size_t ms) {
				assert(in_task());

				(*_tasks_it)->sleeping = new _task_t::_sleep_info_t;
				(*_tasks_it)->sleeping->sleep_to = _cur_time + ms;
				_join_new_task_cor((*_tasks_it)->sleeping->con);
			}

			void wait(const std::function<bool()> &wake_cond) {
				assert(in_task());

				(*_tasks_it)->sleeping = new _task_t::_sleep_info_t;
				(*_tasks_it)->sleeping->wake_cond = wake_cond;
				_join_new_task_cor((*_tasks_it)->sleeping->con);
			}

			void handle_tasks(const std::function<void()> &block_func = []() {
				std::this_thread::sleep_for(std::chrono::milliseconds(0));
			}) {
				if (_handling || !support_co_for_this_thread()) {
					return;
				}
				_handling = true;

				_work_tid = std::this_thread::get_id();

				for (;;) {
					_cur_time = _tick();

					if (_pre_add_tasks_mtx.try_lock()) {
						for (_pre_add_tasks.size()) {
							_tasks.push_back(_pre_add_tasks.top());
							_pre_add_tasks.pop();
						}
						_pre_add_tasks_mtx.unlock();
					}

					_tasks_it = _tasks.begin();

					_in_task_cors = true;
					_join_new_task_cor(_main_con);
					_in_task_cors = false;

					if (block_func) {
						block_func();
					}
				}

				_handling = false;
			}

			bool in_task() {
				return _in_work_td() && _in_task_cors;
			}

		private:
			std::thread::id _work_tid = std::this_thread::get_id();
			bool _in_work_td() {
				return std::this_thread::get_id() == _work_tid;
			}

			bool _handling = false, _in_task_cors = false;

			continuation _main_con;
			std::list<coroutine> _cors;
			std::stack<continuation> _idle_cor_cons;

			struct _task_t {
				std::function<void()> handler;
				size_t del_time;

				struct _sleep_info_t {
					size_t sleep_to;
					std::function<bool()> wake_cond;
					continuation con;
				};
				_sleep_info_t *sleeping;
			};

			typedef std::list<_task_t *> _task_list_t;

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

			std::stack<_task_t *> _pre_add_tasks;
			std::mutex _pre_add_tasks_mtx;

			void _join_new_task_cor(continuation &ccr) {
				if (_idle_cor_cons.empty()) {
					_cors.emplace_back([this]() {
						for (;;) {
							while (_tasks_it != _tasks.end()) {
								if ((*_tasks_it)->sleeping) {
									if ((*_tasks_it)->sleeping->wake_cond) {
										if (!(*_tasks_it)->sleeping->wake_cond()) {
											++_tasks_it;
											continue;
										}
									} else if (_cur_time < (*_tasks_it)->sleeping->sleep_to) {
										++_tasks_it;
										continue;
									}
									auto con = (*_tasks_it)->sleeping->con;
									delete (*_tasks_it)->sleeping;
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
