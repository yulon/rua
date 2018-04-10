#ifndef _RUA_CP_CHAN_HPP
#define _RUA_CP_CHAN_HPP

#include "sched.hpp"

#include "../macros.hpp"

#include <memory>
#include <queue>
#include <mutex>
#include <atomic>
#include <functional>
#include <vector>
#include <cassert>

namespace rua {
	namespace cp {
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
								auto cv = _res->reqs.front();
								_res->reqs.pop();
								_res->mtx.unlock();
								cv->notify();
								return;
							}
							_res->mtx.unlock();
						}

						scoped_stream_t(const scoped_stream_t &) = delete;

						scoped_stream_t &operator=(const scoped_stream_t &) = delete;

						scoped_stream_t(scoped_stream_t &&) = default;

						scoped_stream_t &operator=(scoped_stream_t &&) = default;

						scoped_stream_t &operator<<(T value) {
							assert(_res);

							_res->buffer.emplace(std::move(value));

							return *this;
						}

						template <typename R>
						scoped_stream_t &operator>>(R &receiver) {
							RUA_STATIC_ASSERT((std::is_convertible<T, R>::value));
							assert(_res);

							auto res = _res;

							auto cv = _scdlr->make_cond_var();
							cv->cond_wait(res->mtx, [res, cv]()->bool {
								if (res->buffer.size()) {
									return true;
								}
								res->reqs.emplace(cv);
								return false;
							});

							receiver = std::move(res->buffer.front());
							res->buffer.pop();

							return *this;
						}

						T get() {
							T r;
							*this >> r;
							return r;
						}

						std::queue<T> get_all() {
							std::queue<T> r;
							r.push(get());
							while (_res->buffer.size()) {
								r.push(std::move(_res->buffer.front()));
								_res->buffer.pop();
							}
							return r;
						}

					private:
						std::shared_ptr<_res_t> _res;
						scheduler _scdlr;
				};

				typename chan<T>::scoped_stream_t scoped_stream() const {
					return scoped_stream_t(*this);
				}

				typename chan<T>::scoped_stream_t operator<<(T value) {
					return std::move(scoped_stream() << value);
				}

				template <typename R>
				typename chan<T>::scoped_stream_t operator>>(R &receiver) {
					return std::move(scoped_stream() >> receiver);
				}

				T get() {
					return scoped_stream().get();
				}

				std::queue<T> get_all() {
					return scoped_stream().get_all();
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
}

#endif
