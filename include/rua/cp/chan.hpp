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
				class ioer_t {
					public:
						ioer_t(const chan<T> &ch, bool try_lock = false) {
							if (!ch._res) {
								return;
							}
							if (try_lock) {
								_scdlr = ch._res->mscdlr.try_lock(ch._res->mtx);
								if (_scdlr) {
									_res = ch._res;
								}
							} else {
								_res = ch._res;
								_scdlr = ch._res->mscdlr.lock(ch._res->mtx);
							}
						}

						~ioer_t() {
							unlock();
						}

						ioer_t(const ioer_t &) = delete;

						ioer_t &operator=(const ioer_t &) = delete;

						ioer_t(ioer_t &&) = default;

						ioer_t &operator=(ioer_t &&) = default;

						ioer_t &operator<<(T value) {
							assert(_res);

							_res->buffer.emplace(std::move(value));

							return *this;
						}

						ioer_t &operator<<(std::queue<T> values) {
							assert(_res);

							while (values.size()) {
								_res->buffer.emplace(std::move(values.front()));
								values.pop();
							}

							return *this;
						}

						void wait_inputs() {
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
						}

						template <typename R>
						ioer_t &operator>>(R &receiver) {
							RUA_STATIC_ASSERT((std::is_convertible<T, R>::value));

							wait_inputs();

							receiver = std::move(_res->buffer.front());
							_res->buffer.pop();

							return *this;
						}

						T get() {
							T r;
							*this >> r;
							return r;
						}

						std::queue<T> get_all() {
							wait_inputs();
							return std::move(_res->buffer);
						}

						T try_get() {
							T r;
							if (_res->buffer.size()) {
								r = std::move(_res->buffer.front());
								_res->buffer.pop();
							}
							return r;
						}

						std::queue<T> try_get_all() {
							return std::move(_res->buffer);
						}

						void unlock() {
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

						operator bool() const {
							return _res.get();
						}

					private:
						std::shared_ptr<_res_t> _res;
						scheduler _scdlr;
				};

				typename chan<T>::ioer_t lock() const {
					return ioer_t(*this);
				}

				typename chan<T>::ioer_t try_lock() const {
					return ioer_t(*this, true);
				}

				template <typename V>
				typename chan<T>::ioer_t operator<<(V &&value) {
					auto ioer = lock();
					ioer << std::forward<V>(value);
					return ioer;
				}

				typename chan<T>::ioer_t wait_inputs() {
					auto ioer = lock();
					ioer.wait_inputs();
					return ioer;
				}

				template <typename R>
				typename chan<T>::ioer_t operator>>(R &receiver) {
					auto ioer = lock();
					ioer >> receiver;
					return ioer;
				}

				T get() {
					return lock().get();
				}

				std::queue<T> get_all() {
					return lock().get_all();
				}

				T try_get() {
					auto ioer = try_lock();
					if (ioer) {
						return ioer.try_get_all();
					}
					return T();
				}

				std::queue<T> try_get_all() {
					auto ioer = try_lock();
					if (ioer) {
						return ioer.try_get_all();
					}
					return std::queue<T>();
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
