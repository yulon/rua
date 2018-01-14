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

		private:
			struct _res_t;

		public:
			class scoped_stream_t {
				public:
					scoped_stream_t(const chan<T> &ch) : _res(ch._res), _scdlr(ch._res->mscdlr.lock(ch._res->mtx)) {}

					~scoped_stream_t() {
						if (!_res) {
							return;
						}
						if (_res->buffer.size() && _res->reqs.size()) {
							_res->reqs.front()->notify();
						}
						_res->mtx.unlock();
					}

					scoped_stream_t(const scoped_stream_t &) = delete;

					scoped_stream_t &operator=(const scoped_stream_t &) = delete;

					scoped_stream_t(scoped_stream_t &&) = default;

					scoped_stream_t &operator=(scoped_stream_t &&) = default;

					chan<T>::scoped_stream_t &operator<<(T value) {
						_res->buffer.push(std::move(value));
						return *this;
					}

					template <typename R>
					chan<T>::scoped_stream_t &operator>>(R &receiver) {
						if (!_res->buffer.size()) {
							auto res = _res;
							auto req = _scdlr->make_cond_var();
							res->reqs.emplace(req);
							for (;;) {
								res->mtx.unlock();
								req->cond_wait([res]()->bool {
									return res->mtx.try_lock();
								});
								if (res->buffer.size()) {
									assert(&**res->reqs.front() == &**req);
									res->reqs.pop();
									break;
								}
							};
						}

						receiver = std::move(_res->buffer.front());
						_res->buffer.pop();

						return *this;
					}

				private:
					std::shared_ptr<_res_t> _res;
					scheduler _scdlr;
			};

			chan<T>::scoped_stream_t scoped_stream() const {
				return scoped_stream_t(*this);
			}

			chan<T>::scoped_stream_t operator<<(T value) {
				return std::move(scoped_stream() << value);
			}

			template <typename R>
			chan<T>::scoped_stream_t operator>>(R &receiver) {
				return std::move(scoped_stream() >> receiver);
			}

		private:
			struct _res_t {
				std::mutex mtx;
				multi_scheduler mscdlr;
				std::queue<T> buffer;
				std::queue<cond_var> reqs;
			};

			std::shared_ptr<_res_t> _res;
	};
}

#endif
