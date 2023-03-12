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
	delivered,
	received
};

RUA_CVAR strv_error err_promise_undelivered("promise undelivered");
RUA_CVAR strv_error err_promise_not_yet_delivered("promise not yet delivered");

template <typename T = void>
class promise {
public:
	promise() : _state(promise_state::loss_notify) {}

	promise(const promise &) = delete;
	promise(promise &&) = delete;
	promise &operator=(const promise &) = delete;
	promise &operator=(promise &&) = delete;

	virtual ~promise() {}

	//////////////////// send side ////////////////////

	void deliver(
		expected<T> value = err_promise_undelivered,
		std::function<void(expected<T>)> rewind = nullptr) {

		assert(std::is_void<T>::value ? !!_val : !_val);
		_val = std::move(value);

		if (rewind) {
			assert(!_rewind);
			_rewind = std::move(rewind);
		}

		auto old_state = _state.exchange(promise_state::delivered);
		assert(old_state != promise_state::delivered);

		switch (old_state) {

		case promise_state::has_notify: {
			assert(_notify);
			auto notify = std::move(_notify);
			notify();
			break;
		}

		case promise_state::received:
			if (_rewind) {
				_rewind(std::move(_val));
			};
			release();
			break;

		default:
			break;
		}
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
			old_state == promise_state::delivered);

		return old_state != promise_state::delivered;
	}

	constexpr bool await_ready() const {
		return false;
	}

	expected<T> await_resume() {
		expected<T> r;

		auto old_state = _state.exchange(promise_state::received);
		assert(old_state != promise_state::received);

		if (old_state == promise_state::delivered) {
			r = std::move(_val);
			release();
		} else {
			r = err_promise_not_yet_delivered;
		}
		return r;
	}

protected:
	// called when the promise is done using.
	virtual void release() {}

private:
	std::atomic<promise_state> _state;
	expected<T> _val;
	std::function<void()> _notify;
	std::function<void(expected<T>)> _rewind;
};

template <typename T = void>
class newable_promise : public promise<T> {
public:
	virtual ~newable_promise() = default;

protected:
	void release() override {
		delete this;
	}
};

} // namespace rua

#endif
