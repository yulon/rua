#ifndef _RUA_CO_POOL_HPP
#define _RUA_CO_POOL_HPP

#include "co.hpp"
#include "sched.hpp"

#if defined(_RUA_UNIX_)
	#include <time.h>
#endif

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
#include <cassert>

namespace rua {
	class co_pool {
		public:
			co_pool(size_t coro_stack_size = coro::default_stack_size) :
				_exit_on_empty(true),
				_running(false),
				_co_stk_sz(coro_stack_size),
				_pre_add_tasks_sz(0),
				_notify_all(false)
			{
				init();
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

			static constexpr size_t duration_forever = SIZE_MAX;
			static constexpr size_t duration_disposable = 0;

			task add(task pos, std::function<void()> handler, size_t duration = duration_forever) {
				auto tsk = std::make_shared<_task_info_t>();

				tsk->handler = std::move(handler);
				tsk->sleeping = false;
				tsk->sleep_info.notified = false;

				if (_in_work_td()) {
					tsk->del_time = _dur2time(duration);
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
					tsk->del_time = duration;
					tsk->state = _task_info_t::state_t::adding;

					_oth_td_op_mtx.lock();
					_pre_add_tasks.emplace_front(_pre_task_info_t{tsk, std::move(pos)});
					++_pre_add_tasks_sz;
					_oth_td_op_mtx.unlock();
				}

				return tsk;
			}

			task add(std::function<void()> handler, size_t duration = duration_forever) {
				return add(current(), std::move(handler), duration);
			}

			task go(std::function<void()> handler) {
				return add(std::move(handler), duration_disposable);
			}

			task add_front(std::function<void()> handler, size_t duration = duration_forever) {
				return add(nullptr, std::move(handler), duration);
			}

			task add_back(std::function<void()> handler, size_t duration = duration_forever) {
				return add(back(), std::move(handler), duration);
			}

			void sleep(task tsk, size_t duration) {
				assert(_in_work_td() && has(tsk));

				tsk->sleep_info.wake_time = _dur2time(duration);

				_sleep(tsk);
			}

			void sleep(size_t duration) {
				sleep(current(), duration);
			}

			void yield(task tsk) {
				sleep(tsk, 0);
			}

			void yield() {
				yield(current());
			}

			void cond_wait(task tsk, std::function<bool()> pred, size_t timeout_duration = duration_forever) {
				assert(_in_work_td() && has(tsk));

				if (tsk->sleep_info.notified.exchange(false) && pred()) {
					return;
				}

				tsk->sleep_info.wake_time = _dur2time(timeout_duration);
				tsk->sleep_info.wake_cond = std::move(pred);

				_sleep(tsk);
			}

			void cond_wait(std::function<bool()> pred, size_t timeout_duration = duration_forever) {
				cond_wait(current(), std::move(pred), timeout_duration);
			}

			void notify(task tsk) {
				assert(has(tsk));

				tsk->sleep_info.notified = true;
			}

			void notify() {
				notify(current());
			}

			void notify_all() {
				if (_in_work_td()) {
					for (auto &tsk : _tasks) {
						if (tsk->sleeping && tsk->sleep_info.wake_cond) {
							notify(tsk);
						}
					}
				} else {
					_notify_all = true;
				}
			}

			void reset_dol(task tsk, size_t duration) {
				assert(_in_work_td() && has(tsk));

				tsk->del_time = _dur2time(duration);
			}

			void reset_dol(size_t duration) {
				reset_dol(current(), duration);
			}

			void erase(task tsk) {
				assert(_in_work_td() && has(tsk));

				if (tsk->state == _task_info_t::state_t::added) {
					if (tsk.get() == _tasks_it->get()) {
						reset_dol(tsk, duration_disposable);
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

			bool has(task tsk) const {
				return tsk && tsk->state != _task_info_t::state_t::deleted;
			}

			task current() const {
				return _in_work_td() && _tasks_it != _tasks.end() ? *_tasks_it : nullptr;
			}

			task running() const {
				assert(_in_work_td());

				return _running ? *_tasks_it : nullptr;
			}

			bool is_running() const {
				assert(_in_work_td());

				return _running;
			}

			bool this_caller_in_task() const {
				return _in_work_td() && _running;
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
				assert(_in_work_td() && !is_running());

				_cur_time = _tick();

				if (_pre_add_tasks_sz && _oth_td_op_mtx.try_lock()) {
					while (_pre_add_tasks.size()) {
						auto &pt = _pre_add_tasks.front();
						if (pt.tsk->state == _task_info_t::state_t::adding) {
							pt.tsk->state = _task_info_t::state_t::added;

							pt.tsk->del_time = _dur2time(pt.tsk->del_time);

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

				if (_tasks.size()) {
					_life = true;
					_notified_all = _notify_all.exchange(false);
					_join_new_task_cor(_main_ct);
				}
			}

			size_t size() const {
				assert(_in_work_td());

				return _tasks.size() + _pre_add_tasks_sz.load();
			}

			size_t empty() const {
				return !size();
			}

			size_t coro_total() const {
				assert(_in_work_td());

				return _cos.size();
			}

			void exit() {
				assert(_in_work_td());

				_life = false;
			}

			void exit_on_empty(bool toggle = true) {
				assert(_in_work_td());

				_exit_on_empty = toggle;
			}

			class cond_var_c : public rua::cond_var_c {
				public:
					cond_var_c(co_pool &cp) : _cp(cp), _tsk(cp.current()) {
						assert(cp.this_caller_in_task());
					}

					virtual ~cond_var_c() = default;

					virtual void cond_wait(std::function<bool()> pred) {
						assert(_cp.this_caller_in_task() && _tsk.get() == _cp.current().get());

						_cp.cond_wait(_tsk, pred);
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

			bool _in_work_td() const {
				return std::this_thread::get_id() == _work_tid;
			}

			bool _life, _exit_on_empty, _running;

			size_t _co_stk_sz;
			std::list<coro> _cos;

			cont _main_ct;
			std::stack<cont> _idle_co_cts;

			using _task_list_t = std::list<task>;

			struct _task_info_t {
				std::function<void()> handler;
				size_t del_time;

				struct {
					size_t wake_time;
					std::function<bool()> wake_cond;
					std::atomic<bool> notified;
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

			size_t _dur2time(size_t duration) {
				return duration > duration_forever - _cur_time ? duration_forever : _cur_time + duration;
			}

			bool _is_expiration(size_t time) {
				return time != duration_forever && _cur_time >= time;
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
									_tasks_it = _tasks.begin();
								}

								auto &tsk = *_tasks_it;

								if (tsk->sleeping) {
									if (_is_expiration(tsk->sleep_info.wake_time)) {
										_wake();
										continue;
									}

									if (
										tsk->sleep_info.wake_cond &&
										(tsk->sleep_info.notified.exchange(false) || _notified_all) &&
										tsk->sleep_info.wake_cond()
									) {
										tsk->sleep_info.wake_cond = nullptr;
										_wake();
										continue;
									}

									++_tasks_it;
									continue;
								}

								_running = true;
								tsk->handler();
								_running = false;

								if (_is_expiration(tsk->del_time)) {
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
