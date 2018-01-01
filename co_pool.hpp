#ifndef _RUA_CO_POOL_HPP
#define _RUA_CO_POOL_HPP

#include "co.hpp"

#if defined(_RUA_UNIX_)
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

namespace rua {
	class co_pool {
		public:
			co_pool(size_t coro_stack_size = coro::default_stack_size) :
				_co_stk_sz(coro_stack_size), _pre_add_tasks_sz(0)
			{
				_tasks_it = _tasks.end();
			}

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

			task add(task pos, const std::function<void()> &handler, size_t duration_of_life = duration_always) {
				auto tsk = std::make_shared<_task_info_t>();

				tsk->handler = handler;

				if (_in_work_td()) {
					tsk->del_time = duration_of_life == duration_always ? duration_always : _cur_time + duration_of_life;
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
					tsk->del_time = duration_of_life;
					tsk->state = _task_info_t::state_t::adding;

					_oth_td_op_mtx.lock();
					_pre_add_tasks.emplace(_pre_task_info_t{tsk, std::move(pos)});
					++_pre_add_tasks_sz;
					_oth_td_op_mtx.unlock();
				}

				return tsk;
			}

			task add(const std::function<void()> &handler, size_t duration_of_life = duration_always) {
				return add(current(), handler, duration_of_life);
			}

			task go(const std::function<void()> &handler) {
				return add(handler, duration_disposable);
			}

			task add_front(const std::function<void()> &handler, size_t duration_of_life = duration_always) {
				return add(nullptr, handler, duration_of_life);
			}

			task add_back(const std::function<void()> &handler, size_t duration_of_life = duration_always) {
				return add(back(), handler, duration_of_life);
			}

			void sleep(task tsk, size_t ms) {
				assert(_in_work_td());

				if (!has(tsk)) {
					return;
				}

				tsk->sleeping.sleep_to = _cur_time + ms;
				if (is_running() && tsk.get() == _tasks_it->get()) {
					tsk.reset();
					_sleep_running();
				}
			}

			void sleep(size_t ms) {
				assert(_in_work_td());

				sleep(current(), ms);
			}

			void wait(task tsk, const std::function<bool()> &wake_cond) {
				assert(_in_work_td());

				if (!has(tsk)) {
					return;
				}

				tsk->sleeping.wake_cond = wake_cond;
				if (is_running() && tsk.get() == _tasks_it->get()) {
					tsk.reset();
					_sleep_running();
				}
			}

			void wait(const std::function<bool()> &wake_cond) {
				assert(_in_work_td());

				wait(current(), wake_cond);
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

				if (!has(tsk)) {
					return;
				}

				tsk->del_time = duration_of_life == duration_always ? duration_always : _cur_time + duration_of_life;
			}

			void reset_dol(size_t duration_of_life) {
				assert(_in_work_td());

				reset_dol(current(), duration_of_life);
			}

			void erase(task tsk) {
				assert(_in_work_td());

				if (!has(tsk)) {
					return;
				}

				if (tsk->state == _task_info_t::state_t::added) {
					if (tsk.get() == _tasks_it->get()) {
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

				} else {
					tsk->state = _task_info_t::state_t::deleted;
				}
			}

			void erase() {
				assert(_in_work_td());

				erase(current());
			}

			bool has(task tsk) const {
				assert(_in_work_td());

				return tsk && tsk->state != _task_info_t::state_t::deleted;
			}

			task current() const {
				return _in_work_td() && _tasks_it != _tasks.end() ? *_tasks_it : nullptr;
			}

			task running() const {
				assert(_in_work_td());

				return _is_running ? *_tasks_it : nullptr;
			}

			bool is_running() const {
				assert(_in_work_td());

				return _is_running;
			}

			bool this_caller_in_task() const {
				return _in_work_td() && _is_running;
			}

			task front() const {
				assert(_in_work_td());

				return _tasks.size() ? _tasks.front() : nullptr;
			}

			task back() const {
				assert(_in_work_td());

				return _tasks.size() ? _tasks.back() : nullptr;
			}

			void init() {
				_work_tid = std::this_thread::get_id();
			}

			void handle() {
				assert(!is_running());

				_cur_time = _tick();

				if (_pre_add_tasks_sz && _oth_td_op_mtx.try_lock()) {
					while (_pre_add_tasks.size()) {
						auto &pt = _pre_add_tasks.front();
						if (pt.tsk->state == _task_info_t::state_t::adding) {
							pt.tsk->state = _task_info_t::state_t::added;

							pt.tsk->del_time = pt.tsk->del_time == duration_always ? duration_always : _cur_time + pt.tsk->del_time;

							if (has(pt.pos)) {
								auto pos_it = pt.pos->it;
								_tasks.insert(++pos_it, std::move(pt.tsk));
								(*(--pos_it))->it = pos_it;
							} else {
								_tasks.emplace_front(std::move(pt.tsk));
								_tasks.front()->it = _tasks.begin();
							}
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

			bool _is_running = false;

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
				#elif defined(_RUA_UNIX_)
					timespec ts;
					clock_gettime(CLOCK_MONOTONIC, &ts);
					return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
				#endif
			}

			std::mutex _oth_td_op_mtx;

			struct _pre_task_info_t {
				task tsk, pos;
			};

			std::queue<_pre_task_info_t> _pre_add_tasks;
			std::atomic<size_t> _pre_add_tasks_sz;

			void _sleep_running() {
				assert(is_running());

				_is_running = false;
				_join_new_task_cor((*_tasks_it++)->sleeping.ct);
			}

			void _wake() {
				_is_running = true;
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

								_is_running = true;
								tsk->handler();
								_is_running = false;

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
