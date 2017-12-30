#ifndef _RUA_CHAN_HPP
#define _RUA_CHAN_HPP

#include <memory>
#include <queue>
#include <mutex>
#include <atomic>
#include <functional>
#include <vector>

namespace rua {
	template <typename T>
	class chan {
		public:
			struct scheduler_t {
				std::function<bool()> is_available;
				std::function<void()> yield;
				std::function<void(const std::function<bool()> &)> cond_wait;
			};

			chan() : _res(std::make_shared<_res_t>()) {}

			chan(const std::vector<scheduler_t> &schedulers) : chan() {
				_res->schedulers = schedulers;
			}

			chan<T> &operator<<(const T &value) {
				_lock(_get_scheduler());

				if (_res->reqs.size()) {
					_res->reqs.front()->value = value;
					_res->reqs.front()->ok = true;
					_res->reqs.pop();
				} else {
					_res->buffer.push(value);
				}

				_res->mtx.unlock();

				return *this;
			}

			chan<T> &operator<<(T &&value) {
				_lock(_get_scheduler());

				if (_res->reqs.size()) {
					_res->reqs.front()->value = std::move(value);
					_res->reqs.front()->ok = true;
					_res->reqs.pop();
				} else {
					_res->buffer.push(std::move(value));
				}

				_res->mtx.unlock();

				return *this;
			}

			chan<T> &operator>>(T &receiver) {
				auto scheduler = _get_scheduler();
				_lock(scheduler);

				if (_res->buffer.size()) {
					receiver = _res->buffer.front();
					_res->buffer.pop();
					_res->mtx.unlock();
					return *this;
				}

				auto req = new _req_t;
				req->ok = false;
				_res->reqs.emplace(req);
				_res->mtx.unlock();

				if (!req->ok) {
					if (scheduler) {
						_cond_wait(*scheduler, [req]()->bool {
							return req->ok;
						});
					} else {
						do {
							std::this_thread::yield();
						} while (!req->ok);
					}
				}

				receiver = std::move(req->value);
				delete req;

				return *this;
			}

		private:
			struct _req_t {
				std::atomic<bool> ok;
				T value;
			};

			struct _res_t {
				std::mutex mtx;
				std::vector<scheduler_t> schedulers;
				std::queue<T> buffer;
				std::queue<_req_t *> reqs;
			};

			std::shared_ptr<_res_t> _res;

			scheduler_t *_get_scheduler() {
				for (auto &scheduler : _res->schedulers) {
					if (scheduler.is_available()) {
						return &scheduler;
					}
				}
				return nullptr;
			}

			void _cond_wait(scheduler_t &scheduler, const std::function<bool()> &cond) {
				if (scheduler.cond_wait) {
					scheduler.cond_wait(cond);
					return;
				}

				assert(scheduler.yield);

				do {
					scheduler.yield();
				} while (!cond());
			}

			void _lock(scheduler_t *scheduler) {
				if (!_res->mtx.try_lock()) {
					if (scheduler) {
						auto res = _res;
						_cond_wait(*scheduler, [res]()->bool {
							return res->mtx.try_lock();
						});
						return;
					}
					_res->mtx.lock();
				}
			}
	};
}

#endif
