#ifndef _RUA_SYNC_PROMISE_HPP
#define _RUA_SYNC_PROMISE_HPP

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
	promise() : _state(promise_state::loss_notify) {}

	promise(const promise &) = delete;
	promise(promise &&) = delete;
	promise &operator=(const promise &) = delete;
	promise &operator=(promise &&) = delete;

	virtual ~promise() = default;

	/////////////////// fulfill side ///////////////////

	expected<T> fulfill(expected<T> value = err_promise_breaked) {
		assert(std::is_void<T>::value ? !!_val : !_val);

		_val = std::move(value);

		auto old_state = _state.exchange(promise_state::fulfilled);

		assert(old_state != promise_state::fulfilled);

		switch (old_state) {

		case promise_state::has_notify: {
			assert(_notify);
			auto notify = std::move(_notify);
			notify();
			break;
		}

		case promise_state::received: {
			auto r = std::move(_val);
			release();
			return std::move(r);
		}

		default:
			break;
		}

		return err_promise_fulfilled;
	}

	/////////////////// receive side ///////////////////

	bool await_suspend(std::function<void()> notify) {
		_notify = std::move(notify);

		auto old_state = _state.load();
		while (
			old_state == promise_state::loss_notify &&
			!_state.compare_exchange_weak(old_state, promise_state::has_notify))
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

		auto old_state = _state.exchange(promise_state::received);
		assert(old_state != promise_state::received);

		if (old_state == promise_state::fulfilled) {
			r = std::move(_val);
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
	std::atomic<promise_state> _state;
	expected<T> _val;
	std::function<void()> _notify;
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
