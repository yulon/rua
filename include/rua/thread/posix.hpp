#ifndef _RUA_THREAD_POSIX_HPP
#define _RUA_THREAD_POSIX_HPP

#include "../any_word.hpp"
#include "../sched/scheduler.hpp"

#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <unistd.h>

#include <functional>
#include <memory>

namespace rua { namespace posix {

class thread {
public:
	using native_handle_t = pthread_t;

	using id_t = pthread_t;

	////////////////////////////////////////////////////////////////

	static id_t current_id() {
		return pthread_self();
	}

	static thread current() {
		return current_id();
	}

	////////////////////////////////////////////////////////////////

	constexpr thread() : _ctx(nullptr) {}

	explicit thread(std::function<void()> fn) {
		id_t id;
		if (pthread_create(
				&id,
				nullptr,
				[](void *p) -> void * {
					auto f = reinterpret_cast<std::function<void()> *>(p);
					(*f)();
					delete f;
					return nullptr;
				},
				reinterpret_cast<void *>(
					new std::function<void()>(std::move(fn))))) {
			return;
		}
		_ctx = std::make_shared<_ctx_t>(id);
	}

	constexpr thread(std::nullptr_t) : thread() {}

	thread(id_t id) : _ctx(std::make_shared<_ctx_t>(id)) {}

	~thread() {
		reset();
	}

	id_t id() const {
		if (!_ctx) {
			return 0;
		}
		return _ctx->id;
	}

	bool operator==(const thread &target) const {
		return id() == target.id();
	}

	bool operator!=(const thread &target) const {
		return id() != target.id();
	}

	native_handle_t native_handle() const {
		if (!_ctx) {
			return 0;
		}
		return _ctx->id;
	}

	explicit operator bool() const {
		if (!_ctx) {
			return 0;
		}
		return _ctx->id;
	}

	void exit(any_word retval = nullptr) {
		if (!_ctx) {
			return;
		}
		if (*this == current()) {
			pthread_exit(retval);
		} else {
			pthread_cancel(_ctx->id);
		}
		reset();
	}

	any_word wait_for_exit() {
		if (!_ctx) {
			return nullptr;
		}
		void *retval;
		if (pthread_join(_ctx->id, &retval)) {
			return nullptr;
		}
		reset();
		return retval;
	}

	void reset() {
		_ctx.reset();
	}

private:
	class _ctx_t {
	public:
		pthread_t id;

		_ctx_t(pthread_t id) : id(id) {}

		~_ctx_t() {
			pthread_detach(id);
		}
	};
	std::shared_ptr<_ctx_t> _ctx;

public:
	class scheduler : public rua::scheduler {
	public:
		static scheduler &instance() {
			static scheduler ds;
			return ds;
		}

		constexpr scheduler(ms yield_dur = 0) : _yield_dur(yield_dur) {}

		virtual ~scheduler() = default;

		virtual void yield() {
			if (_yield_dur > 1_us) {
				::usleep(us(_yield_dur).count());
			}
			for (auto i = 0; i < 3; ++i) {
				if (!sched_yield()) {
					return;
				}
			}
			::usleep(1);
		}

		virtual void sleep(ms timeout) {
			::usleep(us(timeout).count());
		}

		class signaler : public rua::scheduler::signaler {
		public:
			using native_handle_t = sem_t *;

			signaler() {
				_need_close = !sem_init(&_sem, 0, 0);
			}

			virtual ~signaler() {
				if (!_need_close) {
					return;
				}
				sem_destroy(&_sem);
				_need_close = false;
			}

			native_handle_t native_handle() const {
				return &_sem;
			}

			virtual void signal() {
				sem_post(&_sem);
			}

			virtual void reset() {
				while (sem_trywait(&_sem) != EAGAIN)
					;
			}

		private:
			sem_t _sem;
			bool _need_close;
		};

		virtual signaler_i make_signaler() {
			return std::make_shared<signaler>();
		}

		virtual bool wait(signaler_i wkr, ms timeout = duration_max()) {
			assert(wkr.type_is<signaler>());

			timespec ts;
			ts.tv_sec = static_cast<decltype(ts.tv_sec)>(ms.total_seconds());
			ts.tv_nsec =
				static_cast<decltype(ts.tv_nsec)>(ms.extra_nanoseconds());
			return sem_timedwait(wkr.to<signaler>()->native_handle(), &ts) !=
				   ETIMEDOUT;
		}

	private:
		ms _yield_dur;
	};
};

}} // namespace rua::posix

#endif
