#ifndef _RUA_CHAN_HPP
#define _RUA_CHAN_HPP

#include "sched.hpp"

#include <memory>
#include <queue>
#include <mutex>
#include <atomic>
#include <functional>
#include <vector>
#include <cassert>

namespace rua {
	template <typename T>
	class chan {
		public:
			chan() : _res(std::make_shared<_res_t>()) {}

			chan(multi_scheduler mscdlr) : chan() {
				_res->mscdlr = std::move(mscdlr);
			}

			chan<T> &operator<<(T value) {
				_res->mscdlr.lock(_res->mtx);

				if (_res->reqs.size()) {
					auto &req = *_res->reqs.front();
					req.value = std::move(value);
					req.filled = true;
					req.cv->notify();
					_res->reqs.pop();
				} else {
					_res->buffer.push(std::move(value));
				}

				_res->mtx.unlock();

				return *this;
			}

			template <typename R>
			chan<T> &operator>>(R &receiver) {
				auto scdlr = _res->mscdlr.lock(_res->mtx);

				if (_res->buffer.size()) {
					receiver = std::move(_res->buffer.front());
					_res->buffer.pop();
					_res->mtx.unlock();
					return *this;
				}

				auto req = new _req_t;
				_res->reqs.emplace(req);
				req->cv = scdlr->make_cond_var();

				_res->mtx.unlock();

				req->cv->cond_wait([req]()->bool {
					return req->filled;
				});

				receiver = std::move(req->value);
				delete req;

				return *this;
			}

		private:
			struct _req_t {
				std::atomic<bool> filled = false;
				T value;
				cond_var cv;
			};

			struct _res_t {
				std::mutex mtx;
				multi_scheduler mscdlr;
				std::queue<T> buffer;
				std::queue<_req_t *> reqs;
			};

			std::shared_ptr<_res_t> _res;
	};
}

#endif
