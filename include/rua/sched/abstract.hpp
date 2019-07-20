#ifndef _RUA_SCHED_ABSTRACT_HPP
#define _RUA_SCHED_ABSTRACT_HPP

#include "../lock_ref.hpp"
#include "../limits.hpp"
#include "../ref.hpp"

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

	using cond_var_i = ref<cond_var>;

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

using scheduler_i = ref<scheduler>;

}

#endif
