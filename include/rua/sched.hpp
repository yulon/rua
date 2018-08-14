#ifndef _RUA_SCHED_HPP
#define _RUA_SCHED_HPP

#include "tls.hpp"
#include "lock_ref.hpp"
#include "limits.hpp"

#include <chrono>
#include <memory>
#include <type_traits>
#include <thread>
#include <condition_variable>

namespace rua {
	class scheduler {
		public:
			virtual ~scheduler() = default;

			virtual void yield() {
				sleep(0);
			}

			virtual void sleep(size_t timeout) {
				auto tp = std::chrono::steady_clock().now();
				do {
					yield();
				} while (static_cast<size_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock().now() - tp).count()) < timeout);
			}

			virtual void lock(typeless_lock_ref &lck) {
				while (!lck.try_lock()) {
					yield();
				}
			}

			template <
				typename Lockable,
				typename = typename std::enable_if<!std::is_base_of<typeless_lock_ref, typename std::decay<Lockable>::type>::value>::type
			>
			void lock(Lockable &lck) {
				auto &&lck_ref = make_lock_ref(lck);
				lock(lck_ref);
			}

			class cond_var {
				public:
					virtual void notify() {}
			};

			virtual std::shared_ptr<cond_var> make_cond_var() {
				return std::make_shared<cond_var>();
			}

			virtual std::cv_status cond_wait(std::shared_ptr<cond_var>, typeless_lock_ref &, size_t = nmax<size_t>()) {
				yield();
				return std::cv_status::no_timeout;
			}

			template <
				typename Lockable,
				typename = typename std::enable_if<!std::is_base_of<typeless_lock_ref, typename std::decay<Lockable>::type>::value>::type
			>
			std::cv_status cond_wait(std::shared_ptr<cond_var> cv, Lockable &lck, size_t timeout = nmax<size_t>()) {
				auto &&lck_ref = make_lock_ref(lck);
				return cond_wait(cv, lck_ref, timeout);
			}

			template <
				typename Lockable,
				typename Pred,
				typename = typename std::enable_if<!std::is_base_of<typeless_lock_ref, typename std::decay<Lockable>::type>::value>::type
			>
			bool cond_wait(std::shared_ptr<cond_var> cv, Lockable &lck, Pred &&pred, size_t timeout = nmax<size_t>()) {
				auto &&lck_ref = make_lock_ref(lck);
				while (!pred()) {
					if (cond_wait(cv, lck_ref, timeout) == std::cv_status::timeout) {
						return pred();
					}
				}
				return true;
			}

		protected:
			scheduler() {}
	};

	class default_scheduler : public scheduler {
		public:
			static default_scheduler &instance() {
				static default_scheduler ds;
				return ds;
			}

			default_scheduler() = default;
			virtual ~default_scheduler() = default;

			virtual void yield() {
				sleep(10);
			}

			virtual void sleep(size_t timeout) {
				std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
			}

			virtual void lock(typeless_lock_ref &lck) {
				lck.lock();
			}

			class cond_var : public scheduler::cond_var {
				public:
					using native_handle_t = std::condition_variable_any;

					native_handle_t &native_handle() {
						return _cv;
					}

					virtual void notify() {
						_cv.notify_one();
					}

				private:
					std::condition_variable_any _cv;
			};

			virtual std::shared_ptr<scheduler::cond_var> make_cond_var() {
				return std::static_pointer_cast<scheduler::cond_var>(std::make_shared<cond_var>());
			}

			virtual std::cv_status cond_wait(std::shared_ptr<scheduler::cond_var> cv, typeless_lock_ref &lck, size_t timeout = nmax<size_t>()) {
				return static_cast<cond_var &>(*cv).native_handle().wait_for(lck, std::chrono::milliseconds(timeout));
			}
	};

	inline tls &_scheduler_storage() {
		static tls s;
		return s;
	}

	inline scheduler &_get_scheduler(tls &ss) {
		auto cur = ss.get().to<scheduler *>();
		if (!cur) {
			return default_scheduler::instance();
		}
		return *cur;
	}

	inline scheduler &get_scheduler() {
		return _get_scheduler(_scheduler_storage());
	}

	inline scheduler &set_scheduler(scheduler &s) {
		auto &ss = _scheduler_storage();
		auto &_previous = _get_scheduler(ss);
		ss.set(&s);
		return _previous;
	}

	class scheduler_guard {
		public:
			scheduler_guard(scheduler &s) : _previous(set_scheduler(s)) {}

			~scheduler_guard() {
				set_scheduler(_previous);
			}

			scheduler &previous() {
				return _previous;
			}

		private:
			scheduler &_previous;
	};

	void yield() {
		get_scheduler().yield();
	}

	void sleep(size_t timeout) {
		get_scheduler().sleep(timeout);
	}
}

#endif
