#ifndef _rua_sync_promise_hpp
#define _rua_sync_promise_hpp

#include "await.hpp"

#include "../error.hpp"
#include "../util.hpp"

#include <atomic>
#include <cassert>
#include <functional>
#include <memory>

namespace rua {

enum class promise_state : uintptr_t {
	loss_notify,
	has_notify,
	fulfilled,
	received
};

RUA_CVAR strv_error err_promise_breaked("promise breaked");
RUA_CVAR strv_error err_promise_not_yet_fulfilled("promise not yet fulfilled");
RUA_CVAR strv_error err_promise_fulfilled("promise fulfilled");

template <typename T = void>
class promise {
public:
	promise() : $state(promise_state::loss_notify) {}

	promise(const promise &) = delete;
	promise(promise &&) = delete;
	promise &operator=(const promise &) = delete;
	promise &operator=(promise &&) = delete;

	virtual ~promise() = default;

	/////////////////// fulfill side ///////////////////

	expected<T> fulfill(expected<T> value = err_promise_breaked) {
		assert(std::is_void<T>::value ? !!$val : !$val);

		$val = std::move(value);

		auto old_state = $state.exchange(promise_state::fulfilled);

		assert(old_state != promise_state::fulfilled);

		expected<T> r(err_promise_fulfilled);

		switch (old_state) {

		case promise_state::has_notify: {
			assert($notify);
			auto notify = std::move($notify);
			notify();
			break;
		}

		case promise_state::received:
			r = std::move($val);
			release();
			break;

		default:
			break;
		}

		return r;
	}

	/////////////////// receive side ///////////////////

	bool await_suspend(std::function<void()> notify) {
		$notify = std::move(notify);

		auto old_state = $state.load();
		while (
			old_state == promise_state::loss_notify &&
			!$state.compare_exchange_weak(old_state, promise_state::has_notify))
			;

		assert(old_state != promise_state::has_notify);
		assert(old_state != promise_state::received);
		assert(
			old_state == promise_state::loss_notify ||
			old_state == promise_state::fulfilled);

		return old_state != promise_state::fulfilled;
	}

	constexpr bool await_ready() const {
		return false;
	}

	expected<T> await_resume() {
		expected<T> r;

		auto old_state = $state.exchange(promise_state::received);
		assert(old_state != promise_state::received);

		if (old_state == promise_state::fulfilled) {
			r = std::move($val);
			release();
		} else {
			r = err_promise_not_yet_fulfilled;
		}
		return r;
	}

	/////////////////// release side ///////////////////

	/*
		1. If fulfilled and received, will release() automatically.
		2. If unfulfilled and unreceived, need release() manually.
	*/
	virtual void release() {}

private:
	std::atomic<promise_state> $state;
	expected<T> $val;
	std::function<void()> $notify;
};

template <typename T = void>
class newable_promise : public promise<T> {
public:
	virtual ~newable_promise() = default;

	void release() override {
		delete this;
	}
};

} // namespace rua

#endif
