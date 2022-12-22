#ifndef _RUA_SYNC_PROMISE_HPP
#define _RUA_SYNC_PROMISE_HPP

#include "await.hpp"

#include "../error.hpp"
#include "../skater.hpp"
#include "../util.hpp"
#include "../variant.hpp"

#include <atomic>
#include <cassert>
#include <functional>
#include <memory>

namespace rua {

enum class promise_state : uintptr_t {
	loss_notify,
	has_notify,
	delivered,
	reached
};

template <typename T = void>
struct promise_context {
	std::atomic<promise_state> state;
	expected<T> value;
	std::function<void()> notify;
	std::function<void(expected<T>)> rewind;
};

////////////////////////////////////////////////////////////////////////////

template <typename T = void>
using default_promise_deleter = std::default_delete<promise_context<T>>;

////////////////////////////////////////////////////////////////////////////

RUA_CVAR strv_error err_future_not_yet("future not yet");

template <typename T = void, typename Deteler = default_promise_deleter<T>>
class promising_future : private enable_await_operators {
public:
	constexpr promising_future() : _ctx(nullptr) {}

	explicit promising_future(promise_context<T> &ctx) : _ctx(&ctx) {}

	promising_future(promising_future &&src) :
		_ctx(exchange(src._ctx, nullptr)) {}

	RUA_OVERLOAD_ASSIGNMENT(promising_future)

	promise_context<T> *context() const {
		return _ctx;
	}

	explicit operator bool() const {
		return _ctx;
	}

	bool await_suspend(std::function<void()> notify) {
		assert(_ctx);

		_ctx->notify = std::move(notify);

		auto old_state = _ctx->state.load();
		while (old_state == promise_state::loss_notify &&
			   !_ctx->state.compare_exchange_weak(
				   old_state, promise_state::has_notify))
			;

		assert(old_state != promise_state::has_notify);
		assert(old_state != promise_state::reached);
		assert(
			old_state == promise_state::loss_notify ||
			old_state == promise_state::delivered);

		return old_state != promise_state::delivered;
	}

	constexpr bool await_ready() const {
		return false;
	}

	expected<T> await_resume() {
		assert(_ctx);

		expected<T> r;

		auto old_state = _ctx->state.exchange(promise_state::reached);
		assert(old_state != promise_state::reached);

		if (old_state == promise_state::delivered) {
			r = std::move(_ctx->value);
			Deteler{}(_ctx);
		} else {
			r = err_future_not_yet;
		}
		_ctx = nullptr;

		return r;
	}

	void reset() {
		if (!_ctx) {
			return;
		}
		await_resume();
	}

	promise_context<T> *release() {
		return exchange(_ctx, nullptr);
	}

private:
	promise_context<T> *_ctx;
};

////////////////////////////////////////////////////////////////////////////

template <typename T>
class _future_await_resume {
public:
	template <typename PromisingFuture>
	static inline expected<T> get(variant<T, error_i, PromisingFuture> &v) {
		if (v.template type_is<T>()) {
			return std::move(v).template as<T>();
		}
		if (v.template type_is<error_i>()) {
			return std::move(v).template as<error_i>();
		}
		if (v.template type_is<PromisingFuture>()) {
			auto &fut = v.template as<PromisingFuture>();
			return fut.await_resume();
		}
		return unexpected;
	}
};

template <>
template <typename PromisingFuture>
inline expected<void>
_future_await_resume<void>::get(variant<void, error_i, PromisingFuture> &v) {
	if (v.template type_is<void>()) {
		return expected<void>();
	}
	if (v.template type_is<error_i>()) {
		return std::move(v).template as<error_i>();
	}
	if (v.template type_is<PromisingFuture>()) {
		auto &fut = v.template as<PromisingFuture>();
		return fut.await_resume();
	}
	return unexpected;
}

template <
	typename T = void,
	typename PromiseResult = T,
	typename PromiseDeteler = default_promise_deleter<T>>
class future : private enable_await_operators {
public:
	using promising_future_t = promising_future<PromiseResult, PromiseDeteler>;

	constexpr future() : _r() {}

	template <
		typename U,
		typename = enable_if_t<std::is_constructible<
			variant<T, error_i, promising_future_t>,
			U &&>::value>>
	future(U &&val) : _r(std::forward<U>(val)) {}

	explicit future(promise_context<PromiseResult> &prm_ctx) :
		_r(promising_future_t(prm_ctx)) {}

	template <
		typename U,
		typename = enable_if_t<
			!std::is_same<T, U>::value && std::is_convertible<U &&, T>::value &&
			!std::is_convertible<future<U, PromiseResult> &&, T>::value>>
	future(future<U, PromiseResult, PromiseDeteler> &&src) :
		_r(std::move(src._r)) {}

	template <
		typename U,
		typename = enable_if_t<!std::is_convertible<U &&, T>::value>,
		typename = enable_if_t<
			!std::is_convertible<future<U, PromiseResult> &&, T>::value>>
	future(future<U, PromiseResult, PromiseDeteler> &&src) :
		_r(std::move(src._r).template as<promising_future_t>()) {}

	future(future &&src) : _r(std::move(src._r)) {}

	RUA_OVERLOAD_ASSIGNMENT(future);

	bool await_ready() const {
		return !_r.template type_is<promising_future_t>();
	}

	bool await_suspend(std::function<void()> notify) {
		assert(!await_ready());

		return _r.template as<promising_future_t>().await_suspend(
			std::move(notify));
	}

	expected<T> await_resume() {
		return _future_await_resume<T>::get(_r);
	}

	template <typename U>
	future<U, PromiseResult> to() {
		return std::move(*this);
	}

	void reset() {
		_r.reset();
	}

private:
	variant<T, error_i, promising_future_t> _r;
};

////////////////////////////////////////////////////////////////////////////

RUA_CVAR strv_error err_breaked_promise("breaked promise");

template <typename T = void, typename Deteler = default_promise_deleter<T>>
class promise {
public:
	constexpr promise() : _ctx(nullptr) {}

	explicit promise(promise_context<T> &ctx) : _ctx(&ctx) {}

	~promise() {
		reset();
	}

	promise(promise &&src) : _ctx(exchange(src._ctx, nullptr)) {}

	RUA_OVERLOAD_ASSIGNMENT(promise)

	promise_context<T> *context() const {
		return _ctx;
	}

	explicit operator bool() const {
		return _ctx;
	}

	promising_future<T, Deteler> get_promising_future() {
		reset();
		_ctx = new promise_context<T>;
		_ctx->state = promise_state::loss_notify;
		return promising_future<T, Deteler>(*_ctx);
	}

	future<T, T, Deteler> get_future() {
		return get_promising_future();
	}

	void deliver(
		expected<T> value = expected<T>(),
		std::function<void(expected<T>)> rewind = nullptr) {

		assert(_ctx);

		assert(std::is_void<T>::value ? !!_ctx->value : !_ctx->value);
		_ctx->value = std::move(value);

		if (rewind) {
			assert(!_ctx->rewind);
			_ctx->rewind = std::move(rewind);
		}

		auto old_state = _ctx->state.exchange(promise_state::delivered);
		assert(old_state != promise_state::delivered);

		switch (old_state) {

		case promise_state::has_notify: {
			assert(_ctx->notify);
			auto notify = std::move(_ctx->notify);
			notify();
			break;
		}

		case promise_state::reached:
			if (_ctx->rewind) {
				_ctx->rewind(std::move(_ctx->value));
			};
			Deteler{}(_ctx);
			break;

		default:
			break;
		}

		_ctx = nullptr;
	}

	void reset() {
		if (!_ctx) {
			return;
		}

		assert(std::is_void<T>::value ? !!_ctx->value : !_ctx->value);
		_ctx->value = err_breaked_promise;

		auto old_state = _ctx->state.exchange(promise_state::delivered);
		assert(old_state != promise_state::delivered);

		switch (old_state) {
		case promise_state::has_notify:
			_ctx->notify = nullptr;
			break;

		case promise_state::reached:
			Deteler{}(_ctx);
			break;

		default:
			break;
		}

		_ctx = nullptr;
	}

	promise_context<T> *release() {
		return exchange(_ctx, nullptr);
	}

private:
	promise_context<T> *_ctx;
};

} // namespace rua

#endif
