#ifndef _RUA_SCHED_HPP
#define _RUA_SCHED_HPP

#include "tls.hpp"
#include "lock_ref.hpp"
#include "limits.hpp"
#include "blended_ptr.hpp"

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
		cond_var() = default;
		virtual ~cond_var() = default;

		virtual void notify() {}
	};

	using cond_var_i = blended_ptr<cond_var>;

	virtual cond_var_i make_cond_var() {
		return std::make_shared<cond_var>();
	}

	virtual std::cv_status cond_wait(cond_var_i, typeless_lock_ref &, size_t = nmax<size_t>()) {
		yield();
		return std::cv_status::no_timeout;
	}

	template <
		typename Lockable,
		typename = typename std::enable_if<!std::is_base_of<typeless_lock_ref, typename std::decay<Lockable>::type>::value>::type
	>
	std::cv_status cond_wait(cond_var_i cv, Lockable &lck, size_t timeout = nmax<size_t>()) {
		auto &&lck_ref = make_lock_ref(lck);
		return cond_wait(cv, lck_ref, timeout);
	}

	template <
		typename Lockable,
		typename Pred,
		typename = typename std::enable_if<!std::is_base_of<typeless_lock_ref, typename std::decay<Lockable>::type>::value>::type
	>
	bool cond_wait(cond_var_i cv, Lockable &lck, Pred &&pred, size_t timeout = nmax<size_t>()) {
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

using scheduler_i = blended_ptr<scheduler>;

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
		cond_var() = default;
		virtual ~cond_var() = default;

		virtual void notify() {
			_cv.notify_one();
		}

	private:
		std::condition_variable_any _cv;
		friend default_scheduler;
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
};

inline tls &_scheduler_storage() {
	static tls s;
	return s;
}

inline scheduler_i _get_scheduler(tls &ss) {
	auto p = ss.get().to<scheduler_i *>();
	if (!p) {
		return default_scheduler::instance();
	}
	return *p;
}

inline scheduler_i get_scheduler() {
	return _get_scheduler(_scheduler_storage());
}

class scheduler_guard {
public:
	scheduler_guard(scheduler_i s) {
		auto &ss = _scheduler_storage();
		auto p = ss.get().to<scheduler_i *>();
		if (p) {
			_previous = *p;
			*p = std::move(s);
			return;
		}
		p = new scheduler_i(std::move(s));
		ss.set(p);
	}

	~scheduler_guard() {
		auto &ss = _scheduler_storage();

		auto p = ss.get().to<scheduler_i *>();
		assert(p);

		if (!_previous) {
			delete p;
			ss.set(nullptr);
			return;
		}
		*p = std::move(_previous);
	}

	scheduler_i previous() {
		if (!_previous) {
			return default_scheduler::instance();
		}
		return _previous;
	}

private:
	scheduler_i _previous;
};

inline void yield() {
	get_scheduler()->yield();
}

inline void sleep(size_t timeout) {
	get_scheduler()->sleep(timeout);
}

template <typename Lockable>
class lock_guard {
public:
	lock_guard() : _owner(nullptr) {}

	lock_guard(Lockable &lck) : _owner(&lck) {
		get_scheduler()->lock(lck);
	}

	~lock_guard() {
		unlock();
	}

	void unlock() {
		if (_owner) {
			_owner->unlock();
			_owner = nullptr;
		}
	}

private:
	Lockable *_owner;
};

}

#endif
