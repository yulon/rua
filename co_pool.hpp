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
#include <atomic>
#include <cassert>

namespace tmd {
	class co_pool {
		public:
			co_pool(size_t coro_stack_size = coro::default_stack_size) :
				_co_stk_sz(coro_stack_size), _pre_add_tasks_sz(0)
			{}

			co_pool(const co_pool &) = delete;
			co_pool &operator=(const co_pool &) = delete;
			co_pool(co_pool &&) = delete;
			co_pool &operator=(co_pool &&) = delete;

		private:
			struct _task_info_t;

		public:
			typedef std::shared_ptr<_task_info_t> task;

			static constexpr size_t duration_always = -1;
			static constexpr size_t duration_disposable = 0;

			task add(const std::function<void()> &handler, size_t duration_of_life = duration_always) {
				auto tsk = std::make_shared<_task_info_t>();

				tsk->handler = handler;

				if (_in_work_td()) {
					tsk->del_time = duration_of_life == duration_always ? duration_always : _cur_time + duration_of_life;
					tsk->state = _task_info_t::state_t::added;

					_tasks.emplace_back(tsk);
					tsk->it = --_tasks.end();

				} else {
					tsk->del_time = duration_of_life;
					tsk->state = _task_info_t::state_t::adding;

					_oth_td_op_mtx.lock();
					_pre_add_tasks.emplace(tsk);
					++_pre_add_tasks_sz;
					_oth_td_op_mtx.unlock();
				}

				return tsk;
			}

			void go(const std::function<void()> &handler) {
				add(handler, duration_disposable);
			}

			void sleep(task tsk, size_t ms) {
				assert(_in_work_td());

				tsk->sleeping.sleep_to = _cur_time + ms;
				if (in_task() && tsk == *_tasks_it) {
					_sleep_this_task();
				}
			}

			void sleep(size_t ms) {
				sleep(*_tasks_it, ms);
			}

			void wait(task tsk, const std::function<bool()> &wake_cond) {
				assert(_in_work_td());

				tsk->sleeping.wake_cond = wake_cond;
				if (in_task() && tsk == *_tasks_it) {
					_sleep_this_task();
				}
			}

			void wait(const std::function<bool()> &wake_cond) {
				wait(*_tasks_it, wake_cond);
			}

			template <typename T>
			void lock(T &try_locker) {
				if (!try_locker.try_lock()) {
					wait([&try_locker]() {
						return try_locker.try_lock();
					});
				}
			}

			void reset_dol(task tsk, size_t duration_of_life) {
				assert(_in_work_td());

				tsk->del_time = duration_of_life == duration_always ? duration_always : _cur_time + duration_of_life;
			}

			void reset_dol(size_t duration_of_life) {
				reset_dol(*_tasks_it, duration_of_life);
			}

			void erase(task tsk) {
				assert(_in_work_td());

				if (tsk->state == _task_info_t::state_t::added) {
					if (tsk.get() == (*_tasks_it).get()) {
						reset_dol(tsk, duration_disposable);
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

			void erase() {
				erase(*_tasks_it);
			}

			bool has(task tsk) const {
				assert(_in_work_td());

				return tsk->state != _task_info_t::state_t::deleted;
			}

			void has() {
				has(*_tasks_it);
			}

			task running() const {
				assert(_in_work_td());

				return *_tasks_it;
			}

			void init() {
				_work_tid = std::this_thread::get_id();
			}

			void handle_tasks() {
				assert(!in_task());

				_cur_time = _tick();

				if (_pre_add_tasks_sz && _oth_td_op_mtx.try_lock()) {
					while (_pre_add_tasks.size()) {
						auto &tsk = _pre_add_tasks.front();
						if (tsk->state == _task_info_t::state_t::adding) {
							tsk->state = _task_info_t::state_t::added;

							tsk->del_time = tsk->del_time == duration_always ? duration_always : _cur_time + tsk->del_time;

							_tasks.emplace_back(std::move(tsk));
							_tasks.back()->it = --_tasks.end();
						}
						_pre_add_tasks.pop();
						--_pre_add_tasks_sz;
					}
					_oth_td_op_mtx.unlock();
				}

				if (_tasks.size()) {
					_tasks_it = _tasks.begin();
					_join_new_task_cor(_main_ct);
				}
			}

			bool in_task() const {
				return _in_work_td() && _in_task;
			}

			size_t size() const {
				return _tasks.size() + _pre_add_tasks_sz.load();
			}

			size_t coro_total() const {
				return _cos.size();
			}

		private:
			std::thread::id _work_tid = std::this_thread::get_id();

			bool _in_work_td() const {
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

			std::mutex _oth_td_op_mtx;

			std::queue<task> _pre_add_tasks;
			std::atomic<size_t> _pre_add_tasks_sz;

			void _sleep_this_task() {
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

								if (tsk->del_time != duration_always && _cur_time >= tsk->del_time) {
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
