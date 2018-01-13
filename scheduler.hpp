#ifndef _RUA_SCHEDULER_HPP
#define _RUA_SCHEDULER_HPP

#include <functional>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace rua {
	struct scheduler {
		std::function<bool()> is_available;
		std::function<void()> yield;
		std::function<void(std::function<bool()>)> cond_wait;
		std::function<std::function<void()>()> get_notify;
		std::function<void()> notify_all;
	};

	class multi_scheduler {
		public:
			std::vector<scheduler> schedulers;

			multi_scheduler() {
				_init();
			}

			multi_scheduler(std::initializer_list<scheduler> scdlrs) : schedulers(scdlrs) {
				_init();
			}

			const scheduler &get() const {
				for (auto &scdlr : schedulers) {
					if (!scdlr.is_available || scdlr.is_available()) {
						return scdlr;
					}
				}
				return _td_scdlr;
			}

			template <typename L>
			const scheduler &lock(L &lock) {
				auto &scdlr = get();
				if (&scdlr == &_td_scdlr) {
					lock.lock();
					return scdlr;
				}
				while (!lock.try_lock()) {
					scdlr.yield();
				}
				return scdlr;
			}

		private:
			std::condition_variable _td_scdlr_cv;
			std::unique_lock<std::mutex> _td_scdlr_lck;

			scheduler _td_scdlr;

			void _init() {
				_td_scdlr.is_available = nullptr;

				_td_scdlr.yield = std::this_thread::yield;

				_td_scdlr.cond_wait = [this](std::function<bool()> pred) {
					_td_scdlr_cv.wait(_td_scdlr_lck, pred);
					_td_scdlr_lck.unlock();
				};

				_td_scdlr.get_notify = nullptr;

				_td_scdlr.notify_all = [this]() {
					_td_scdlr_cv.notify_all();
				};
			}
	};
}

#endif
