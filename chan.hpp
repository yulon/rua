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

			chan<T> &operator<<(T value) {
				_lock(_get_scheduler());

				if (_res->reqs.size()) {
					_res->reqs.front()->value = std::move(value);
					_res->reqs.front()->filled = true;
					_res->reqs.pop();
				} else {
					_res->buffer.push(std::move(value));
					_res->buffered = true;
				}

				_res->mtx.unlock();

				return *this;
			}

			template <typename R>
			chan<T> &operator>>(R &receiver) {
				auto scheduler = _get_scheduler();

				if (!_res->buffered) {
					auto &buffered = _res->buffered;
					_cond_wait(scheduler, [&buffered]()->bool {
						return buffered;
					});
				}

				_lock(scheduler);

				if (_res->buffer.size()) {
					receiver = std::move(_res->buffer.front());
					_res->buffer.pop();
					_res->buffered = _res->buffer.size();
					_res->mtx.unlock();
					return *this;
				}

				auto req = new _req_t;
				_res->reqs.emplace(req);
				_res->mtx.unlock();

				if (!req->filled) {
					auto &filled = req->filled;
					_cond_wait(scheduler, [&filled]()->bool {
						return filled;
					});
				}

				receiver = std::move(req->value);
				delete req;

				return *this;
			}

		private:
			struct _req_t {
				std::atomic<bool> filled = false;
				T value;
			};

			struct _res_t {
				std::mutex mtx;
				std::vector<scheduler_t> schedulers;
				std::queue<T> buffer;
				std::atomic<bool> buffered = false;
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

			void _cond_wait(scheduler_t *scheduler, const std::function<bool()> &cond) {
				if (!scheduler) {
					do {
						std::this_thread::yield();
					} while (!cond());
					return;
				}

				if (scheduler->cond_wait) {
					scheduler->cond_wait(cond);
					return;
				}

				assert(scheduler->yield);

				do {
					scheduler->yield();
				} while (!cond());
			}

			void _lock(scheduler_t *scheduler) {
				if (!_res->mtx.try_lock()) {
					if (scheduler) {
						auto res = _res;
						_cond_wait(scheduler, [res]()->bool {
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
