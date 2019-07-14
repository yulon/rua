#ifndef _RUA_POSIX_THREAD_HPP
#define _RUA_POSIX_THREAD_HPP

#include "../any_word.hpp"
#include "../sched/abstract.hpp"

#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#include <functional>
#include <memory>

namespace rua {
namespace posix {

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
			[](void *p)->void * {
				auto f = reinterpret_cast<std::function<void()> *>(p);
				(*f)();
				delete f;
				return nullptr;
			},
			reinterpret_cast<void *>(new std::function<void()>(std::move(fn)))
		)) {
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

	operator bool() const {
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

		scheduler() = default;

		scheduler(size_t yield_dur) : _yield_dur(yield_dur) {}

		virtual ~scheduler() = default;

		virtual void yield() {
			if (_yield_dur > 1){
				::usleep(_yield_dur * 1000);
			}
			for (auto i = 0; i < 3; ++i) {
				if (!sched_yield()) {
					return;
				}
			}
			::usleep(1 * 1000);
		}

		virtual void sleep(size_t timeout) {
			::usleep(timeout * 1000);
		}

		virtual void lock(typeless_lock_ref &lck) {
			lck.lock();
		}

		class cond_var : public rua::scheduler::cond_var {
		public:
			cond_var() = default;
			virtual ~cond_var() = default;

			virtual void notify() {
				_cv.notify_one();
			}

		private:
			std::condition_variable_any _cv;
			friend scheduler;
		};

		virtual scheduler::cond_var_i make_cond_var() {
			return std::make_shared<cond_var>();
		}

		virtual std::cv_status cond_wait(scheduler::cond_var_i cv, typeless_lock_ref &lck, size_t timeout = nmax<size_t>()) {
			auto dscv = cv.to<cond_var>();
			if (timeout == nmax<size_t>()) {
				dscv->_cv.wait(lck);
				return std::cv_status::no_timeout;
			}
			return dscv->_cv.wait_for(lck, std::chrono::milliseconds(timeout));
		}

	private:
		size_t _yield_dur;
	};
};

}
}

#endif
