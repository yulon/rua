#ifndef _RUA_THREAD_DARWIN_HPP
#define _RUA_THREAD_DARWIN_HPP

#include "../any_word.hpp"
#include "../limits.hpp"
#include "../sched/scheduler.hpp"

#include <dispatch/dispatch.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#include <functional>
#include <memory>

namespace rua { namespace darwin {

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
				auto us_c = us(_yield_dur).count();
				::usleep(
					static_cast<int64_t>(nmax<int>()) < us_c
						? nmax<int>()
						: static_cast<int>(us_c));
			}
			for (auto i = 0; i < 3; ++i) {
				if (!sched_yield()) {
					return;
				}
			}
			::usleep(1);
		}

		virtual void sleep(ms timeout) {
			auto us_c = us(timeout).count();
			::usleep(
				static_cast<int64_t>(nmax<int>()) < us_c
					? nmax<int>()
					: static_cast<int>(us_c));
		}

		class signaler : public rua::scheduler::signaler {
		public:
			using native_handle_t = dispatch_semaphore_t;

			signaler() : _sem(dispatch_semaphore_create(0)) {}

			virtual ~signaler() {
				if (!_sem) {
					return;
				}
				dispatch_release(_sem);
				_sem = nullptr;
			}

			native_handle_t native_handle() {
				return _sem;
			}

			virtual void signal() {
				dispatch_semaphore_signal(_sem);
			}

		private:
			dispatch_semaphore_t _sem;
		};

		virtual signaler_i make_signaler() {
			return std::make_shared<signaler>();
		}

		virtual bool wait(signaler_i sig, ms timeout = duration_max()) {
			assert(sig.type_is<signaler>());

			return !dispatch_semaphore_wait(
				sig.to<signaler>()->native_handle(),
				static_cast<dispatch_time_t>(timeout.extra_nanoseconds()));
		}

	private:
		ms _yield_dur;
	};
};

}} // namespace rua::darwin

#endif
