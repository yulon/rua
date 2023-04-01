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

RUA_CVAR strv_error err_promise_unfulfilled("promise unfulfilled");
RUA_CVAR strv_error err_promise_not_yet_fulfilled("promise not yet fulfilled");
RUA_CVAR strv_error err_promise_fulfilled("promise fulfilled");

template <typename T = void, typename Extend = void>
class promise;

template <typename T>
class promise<T, void> {
public:
	promise() : $state(promise_state::loss_notify) {}

	promise(const promise &) = delete;
	promise(promise &&) = delete;
	promise &operator=(const promise &) = delete;
	promise &operator=(promise &&) = delete;

	virtual ~promise() = default;

	/////////////////// fulfill side ///////////////////

	expected<T> fulfill(
		expected<T> value = expected_or<T>(err_promise_unfulfilled)) noexcept {

		assert(std::is_void<T>::value || !$val);

		$val = std::move(value);

		auto old_state = $state.exchange(promise_state::fulfilled);

		assert(old_state != promise_state::fulfilled);

		expected<T> refunded;

		switch (old_state) {

		case promise_state::received:
			refunded = std::move($val);
			destroy();
			break;

		case promise_state::has_notify: {
			assert($notify);
			auto notify = std::move($notify);
			notify();
			RUA_FALLTHROUGH;
		}

		default:
			refunded = err_promise_fulfilled;
			break;
		}

		return refunded;
	}

	void unfulfill() noexcept {
		fulfill(err_promise_unfulfilled);
	}

	/////////////////// receive side ///////////////////

	bool await_ready() const noexcept {
		assert($state.load() != promise_state::received);

		return $state.load() == promise_state::fulfilled;
	}

	bool await_suspend(std::function<void()> notify) noexcept {
		$notify = std::move(notify);

		auto old_state = $state.load();
		if (old_state == promise_state::loss_notify) {
			$state.compare_exchange_strong(
				old_state, promise_state::has_notify);
		}

		assert(old_state != promise_state::has_notify);
		assert(old_state != promise_state::received);
		assert(
			old_state == promise_state::loss_notify ||
			old_state == promise_state::fulfilled);

		return old_state != promise_state::fulfilled;
	}

	expected<T> await_resume() noexcept {
		expected<T> r;

		auto old_state = $state.exchange(promise_state::received);

		assert(old_state != promise_state::received);

		if (old_state == promise_state::fulfilled) {
			r = std::move($val);
			destroy();
		} else {
			r = err_promise_not_yet_fulfilled;
		}
		return r;
	}

	/////////////////// destroy side ///////////////////

	/*
		1. If fulfilled and received, will destroy() automatically.
		2. If unfulfilled and unreceived, need destroy() manually.
	*/
	virtual void destroy() noexcept {}

private:
	std::atomic<promise_state> $state;
	expected<T> $val;
	std::function<void()> $notify;
};

template <typename T, typename Extend>
class promise : public promise<T, void> {
public:
	promise() = default;

	RUA_TMPL_FWD_CTOR(Args, Extend, promise)
	explicit promise(Args &&...args) :
		promise<T, void>(), $ext(std::forward<Args>(args)...) {}

	RUA_TMPL_FWD_CTOR_IL(U, Args, Extend)
	explicit promise(std::initializer_list<U> il, Args &&...args) :
		promise<T, void>(), $ext(il, std::forward<Args>(args)...) {}

	virtual ~promise() = default;

	Extend &extend() & {
		return $ext;
	}

	Extend &&extend() && {
		return std::move($ext);
	}

	const Extend &extend() const & {
		return $ext;
	}

private:
	Extend $ext;
};

template <typename T = void, typename Extend = void>
class newable_promise : public promise<T, Extend> {
public:
	newable_promise() = default;

	RUA_TMPL_FWD_CTOR(Args, RUA_A(promise<T, Extend>), newable_promise)
	explicit newable_promise(Args &&...args) :
		promise<T, Extend>(std::forward<Args>(args)...) {}

	RUA_TMPL_FWD_CTOR_IL(U, Args, RUA_A(promise<T, Extend>))
	explicit newable_promise(std::initializer_list<U> il, Args &&...args) :
		promise<T, Extend>(il, std::forward<Args>(args)...) {}

	virtual ~newable_promise() = default;

	void destroy() noexcept override {
		delete this;
	}
};

} // namespace rua

#endif
