#ifndef _RUA_SYNC_PROMISE_HPP
#define _RUA_SYNC_PROMISE_HPP

#include "await.hpp"

#include "../optional.hpp"
#include "../skater.hpp"
#include "../util.hpp"
#include "../variant.hpp"

#include <atomic>
#include <cassert>
#include <functional>
#include <memory>

namespace rua {

enum class promise_state : uintptr_t {
	no_callback,
	has_callback,
	has_value,
	done
};

template <typename T>
struct promise_context_base {
	std::atomic<promise_state> state;
	std::function<void()> callback;
};

template <typename T = void>
struct promise_context : promise_context_base<T> {
	optional<T> value;
	std::function<void(T)> undo;
};

template <>
struct promise_context<void> : promise_context_base<void> {
	std::function<void()> undo;
};

////////////////////////////////////////////////////////////////////////////

template <typename T>
using default_promise_deleter = std::default_delete<promise_context<T>>;

////////////////////////////////////////////////////////////////////////////

template <typename T, typename Deteler>
class future_base : private enable_await_operators {
public:
	future_base(future_base &&src) : _ctx(exchange(src._ctx, nullptr)) {}

	RUA_OVERLOAD_ASSIGNMENT_R(future_base)

	promise_context<T> *context() const {
		return _ctx;
	}

	explicit operator bool() const {
		return _ctx;
	}

	promise_state try_set_callback(std::function<void()> callback) {
		assert(_ctx);

		_ctx->callback = std::move(callback);

		auto old_state = _ctx->state.load();
		while (old_state == promise_state::no_callback &&
			   !_ctx->state.compare_exchange_weak(
				   old_state, promise_state::has_callback))
			;
		assert(old_state != promise_state::has_callback);

		switch (old_state) {
		case promise_state::no_callback:
			return promise_state::has_callback;

		case promise_state::done:
			Deteler{}(_ctx);
			_ctx = nullptr;
			break;

		default:
			break;
		}

		return old_state;
	}

	promise_context<T> *release() {
		return exchange(_ctx, nullptr);
	}

	constexpr bool await_ready() const {
		return false;
	}

	template <typename Resume>
	bool await_suspend(Resume resume) {
		assert(_ctx);
#ifdef NDEBUG
		return try_set_callback(std::move(resume)) ==
			   promise_state::has_callback;
#else
		auto state = try_set_callback(std::move(resume));
		if (state == promise_state::has_callback) {
			return true;
		}
		assert(state == promise_state::has_value);
		return false;
#endif
	}

protected:
	promise_context<T> *_ctx;

	constexpr future_base() : _ctx(nullptr) {}

	explicit future_base(promise_context<T> &ctx) : _ctx(&ctx) {}
};

template <typename T = void, typename Deteler = default_promise_deleter<T>>
class promising_future : public future_base<T, Deteler> {
public:
	constexpr promising_future() : future_base<T, Deteler>() {}

	explicit promising_future(promise_context<T> &ctx) :
		future_base<T, Deteler>(ctx) {}

	~promising_future() {
		reset();
	}

	promising_future(promising_future &&src) :
		future_base<T, Deteler>(
			static_cast<future_base<T, Deteler> &&>(std::move(src))) {}

	RUA_OVERLOAD_ASSIGNMENT_R(promising_future)

	optional<T> checkout() {
		optional<T> r;

		auto &ctx = this->_ctx;
		assert(ctx);

		switch (ctx->state.exchange(promise_state::done)) {

		case promise_state::has_value:
			r = std::move(ctx->value);
			RUA_FALLTHROUGH;

		case promise_state::done:
			Deteler{}(ctx);
			break;

		default:
			break;
		}

		ctx = nullptr;
		return r;
	}

	bool checkout_or_lose_promise(bool lose_when_setting_value) {
		if (lose_when_setting_value) {
			return checkout();
		}
		auto &ctx = this->_ctx;
		auto r = checkout();
		if (!r) {
			Deteler{}(ctx);
			ctx = nullptr;
		}
		return r;
	}

	T await_resume() {
		assert(this->_ctx);
#ifdef NDEBUG
		return *checkout();
#else
		auto r = checkout();
		assert(r);
		return *std::move(r);
#endif
	}

	void reset() {
		auto &ctx = this->_ctx;
		if (!ctx) {
			return;
		}

		switch (ctx->state.exchange(promise_state::done)) {

		case promise_state::has_value:
			if (ctx->undo) {
				ctx->undo(*std::move(ctx->value));
			}
			RUA_FALLTHROUGH;

		case promise_state::done:
			Deteler{}(ctx);
			break;

		default:
			break;
		}

		ctx = nullptr;
	}
};

template <typename Deteler>
class promising_future<void, Deteler> : public future_base<void, Deteler> {
public:
	constexpr promising_future() : future_base<void, Deteler>() {}

	explicit promising_future(promise_context<void> &ctx) :
		future_base<void, Deteler>(ctx) {}

	~promising_future() {
		reset();
	}

	promising_future(promising_future &&src) :
		future_base<void, Deteler>(
			static_cast<future_base<void, Deteler> &&>(std::move(src))) {}

	RUA_OVERLOAD_ASSIGNMENT_R(promising_future)

	bool checkout() {
		auto &ctx = this->_ctx;
		assert(ctx);

		switch (ctx->state.exchange(promise_state::done)) {

		case promise_state::has_value:
			return true;

		case promise_state::done:
			Deteler{}(ctx);
			break;

		default:
			break;
		}

		ctx = nullptr;
		return false;
	}

	bool checkout_or_lose_promise(bool lose_when_setting_value) {
		if (lose_when_setting_value) {
			return checkout();
		}
		auto &ctx = this->_ctx;
		auto r = checkout();
		if (!r) {
			Deteler{}(ctx);
			ctx = nullptr;
		}
		return r;
	}

	void await_resume() const {}

	void reset() {
		auto &ctx = this->_ctx;
		if (!ctx) {
			return;
		}

		switch (ctx->state.exchange(promise_state::done)) {

		case promise_state::has_value:
			if (ctx->undo) {
				ctx->undo();
			}
			RUA_FALLTHROUGH;

		case promise_state::done:
			Deteler{}(ctx);
			break;

		default:
			break;
		}

		ctx = nullptr;
	}
};

////////////////////////////////////////////////////////////////////////////

template <
	typename T = void,
	typename PromiseResult = T,
	typename PromiseDeteler = default_promise_deleter<T>>
class future : private enable_await_operators {
public:
	constexpr future() : _r() {}

	template <
		typename... Result,
		typename Front = decay_t<front_t<Result &&...>>,
		typename = enable_if_t<
			std::is_constructible<T, Result &&...>::value &&
			(sizeof...(Result) > 1 ||
			 (!std::is_base_of<future, Front>::value &&
			  !std::is_base_of<
				  promising_future<PromiseResult, PromiseDeteler>,
				  Front>::value))>>
	future(Result &&...result) : _r(std::forward<Result>(result)...) {}

	future(promising_future<PromiseResult, PromiseDeteler> pms_fut) :
		_r(std::move(pms_fut)) {}

	explicit future(promise_context<PromiseResult> &pms_ctx) :
		_r(promising_future<PromiseResult, PromiseDeteler>(pms_ctx)) {}

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
		_r(std::move(src._r)
			   .template as<
				   promising_future<PromiseResult, PromiseDeteler>>()) {}

	future(future &&src) : _r(std::move(src._r)) {}

	RUA_OVERLOAD_ASSIGNMENT_R(future);

	bool await_ready() const {
		return _r.template type_is<T>();
	}

	template <typename Resume>
	bool await_suspend(Resume resume) {
		assert((_r.template type_is<
				promising_future<PromiseResult, PromiseDeteler>>()));

		return _r.template as<promising_future<PromiseResult, PromiseDeteler>>()
			.await_suspend(std::move(resume));
	}

	T await_resume() {
		if (_r.template type_is<
				promising_future<PromiseResult, PromiseDeteler>>()) {
			auto &fut = _r.template as<
				promising_future<PromiseResult, PromiseDeteler>>();
			return static_cast<T>(fut.await_resume());
		}
		return std::move(_r).template as<T>();
	}

	template <typename U>
	future<U, PromiseResult> to() {
		return std::move(*this);
	}

	void reset() {
		_r.reset();
	}

private:
	variant<T, promising_future<PromiseResult, PromiseDeteler>> _r;
};

template <typename PromiseDeteler>
class future<void, void, PromiseDeteler> : private enable_await_operators {
public:
	constexpr future(std::nullptr_t = nullptr) : _pms_fut() {}

	future(promising_future<void, PromiseDeteler> pms_fut) :
		_pms_fut(std::move(pms_fut)) {}

	explicit future(promise_context<> &pms_ctx) : _pms_fut(pms_ctx) {}

	future(future &&src) : _pms_fut(std::move(src._pms_fut)) {}

	RUA_OVERLOAD_ASSIGNMENT_R(future);

	bool await_ready() const {
		return !_pms_fut;
	}

	template <typename Resume>
	bool await_suspend(Resume resume) {
		assert(_pms_fut);

		return _pms_fut.await_suspend(std::move(resume));
	}

	void await_resume() const {}

	void reset() {
		_pms_fut.reset();
	}

private:
	promising_future<void, PromiseDeteler> _pms_fut;
};

template <typename T, typename PromiseDeteler>
class future<T, void, PromiseDeteler> {};

////////////////////////////////////////////////////////////////////////////

template <typename T, typename Deteler>
class promise_base {
public:
	~promise_base() {
		reset();
	}

	promise_base(promise_base &&src) : _ctx(exchange(src._ctx, nullptr)) {}

	RUA_OVERLOAD_ASSIGNMENT_R(promise_base)

	promise_context<T> *context() const {
		return _ctx;
	}

	explicit operator bool() const {
		return _ctx;
	}

	promising_future<T, Deteler> get_promising_future() {
		reset();
		auto &ctx = this->_ctx;
		ctx = new promise_context<T>;
		ctx->state = promise_state::no_callback;
		return promising_future<T, Deteler>(*ctx);
	}

	future<T, T, Deteler> get_future() {
		return get_promising_future();
	}

	void reset() {
		if (!_ctx) {
			return;
		}

		auto old_state = _ctx->state.exchange(promise_state::done);
		assert(old_state != promise_state::has_value);

		switch (old_state) {
		case promise_state::has_callback:
			_ctx->callback = nullptr;
			break;

		case promise_state::done:
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

protected:
	promise_context<T> *_ctx;

	constexpr promise_base() : _ctx(nullptr) {}

	explicit promise_base(promise_context<T> &ctx) : _ctx(&ctx) {}
};

template <typename T = void, typename Deteler = default_promise_deleter<T>>
class promise : public promise_base<T, Deteler> {
public:
	constexpr promise() = default;

	explicit promise(promise_context<T> &ctx) : promise_base<T, Deteler>(ctx) {}

	void fulfill(T value, std::function<void(T)> undo = nullptr) {
		auto &ctx = this->_ctx;
		assert(ctx);

		assert(!ctx->value);
		ctx->value.emplace(std::move(value));

		if (undo) {
			assert(!ctx->undo);
			ctx->undo = std::move(undo);
		}

		auto old_state = ctx->state.exchange(promise_state::has_value);
		assert(old_state != promise_state::has_value);

		switch (old_state) {
		case promise_state::has_callback: {
			assert(ctx->callback);
			auto callback = std::move(ctx->callback);
			callback();
			break;
		}

		case promise_state::done:
			if (ctx->undo) {
				ctx->undo(*std::move(ctx->value));
			};
			Deteler{}(ctx);
			break;

		default:
			break;
		}

		ctx = nullptr;
	}
};

template <typename Deteler>
class promise<void, Deteler> : public promise_base<void, Deteler> {
public:
	constexpr promise() = default;

	explicit promise(promise_context<void> &ctx) :
		promise_base<void, Deteler>(ctx) {}

	void fulfill(std::function<void()> undo = nullptr) {
		auto &ctx = this->_ctx;
		assert(ctx);

		if (undo) {
			assert(!ctx->undo);
			ctx->undo = std::move(undo);
		}

		auto old_state = ctx->state.exchange(promise_state::has_value);
		assert(old_state != promise_state::has_value);

		switch (old_state) {
		case promise_state::has_callback:
			assert(ctx->callback);
			ctx->callback();
			break;

		case promise_state::done:
			if (ctx->undo) {
				ctx->undo();
			};
			Deteler{}(ctx);
			break;

		default:
			break;
		}

		ctx = nullptr;
	}
};

} // namespace rua

#endif
