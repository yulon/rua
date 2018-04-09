#ifndef _RUA_CONCURRENT_SCHED_HPP
#define _RUA_CONCURRENT_SCHED_HPP

#include "../poly.hpp"

#include <functional>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace rua {
	class cond_var_c {
		public:
			virtual ~cond_var_c() = default;

			virtual void cond_wait(std::mutex &lock, std::function<bool()>) = 0;
			virtual void notify() = 0;
	};

	using cond_var = itf<cond_var_c>;

	class scheduler_c {
		public:
			virtual ~scheduler_c() = default;

			virtual bool is_available() const = 0;
			virtual void yield() const = 0;
			virtual cond_var make_cond_var() const = 0;
	};

	using scheduler = itf<scheduler_c>;

	class _thread_cond_var_c : public cond_var_c {
		public:
			_thread_cond_var_c() = default;
			virtual ~_thread_cond_var_c() = default;

			virtual void cond_wait(std::mutex &lock, std::function<bool()> pred) {
				cv.wait(lock, pred);
			}

			virtual void notify() {
				cv.notify_one();
			}

		private:
			std::condition_variable_any cv;
	};

	using thread_cond_var = obj<_thread_cond_var_c>;

	class _thread_scheduler_c : public scheduler_c {
		public:
			_thread_scheduler_c() = default;
			virtual ~_thread_scheduler_c() = default;

			virtual bool is_available() const {
				return true;
			}

			virtual void yield() const {
				std::this_thread::yield();
			}

			virtual cond_var make_cond_var() const {
				return thread_cond_var();
			}
	};

	using thread_scheduler = obj<_thread_scheduler_c>;

	class multi_scheduler {
		public:
			std::vector<scheduler> schedulers;

			multi_scheduler() = default;

			multi_scheduler(std::initializer_list<scheduler> scdlrs) : schedulers(scdlrs) {}

			scheduler get() const {
				for (auto &scdlr : schedulers) {
					if (scdlr->is_available()) {
						return scdlr;
					}
				}
				static scheduler td_scdlr = thread_scheduler();
				return td_scdlr;
			}

			template <typename L>
			scheduler lock(L &lock) {
				auto scdlr = get();
				if (scdlr.type_is<thread_scheduler>()) {
					lock.lock();
					return scdlr;
				}
				while (!lock.try_lock()) {
					scdlr->yield();
				}
				return scdlr;
			}
	};
}

#endif
