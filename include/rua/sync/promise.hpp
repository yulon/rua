#ifndef _RUA_SYNC_PROMISE_HPP
#define _RUA_SYNC_PROMISE_HPP

#include "wait.hpp"

#include "../optional.hpp"
#include "../skater.hpp"
#include "../util.hpp"

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
class future_base : private enable_wait_operator {
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
class future : public future_base<T, Deteler> {
public:
	constexpr future() : future_base<T, Deteler>() {}

	explicit future(promise_context<T> &ctx) : future_base<T, Deteler>(ctx) {}

	~future() {
		reset();
	}

	future(future &&src) :
		future_base<T, Deteler>(
			static_cast<future_base<T, Deteler> &&>(std::move(src))) {}

	RUA_OVERLOAD_ASSIGNMENT_R(future)

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
class future<void, Deteler> : public future_base<void, Deteler> {
public:
	constexpr future() : future_base<void, Deteler>() {}

	explicit future(promise_context<void> &ctx) :
		future_base<void, Deteler>(ctx) {}

	~future() {
		reset();
	}

	future(future &&src) :
		future_base<void, Deteler>(
			static_cast<future_base<void, Deteler> &&>(std::move(src))) {}

	RUA_OVERLOAD_ASSIGNMENT_R(future)

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

	future<T, Deteler> get_future() {
		reset();
		auto &ctx = this->_ctx;
		ctx = new promise_context<T>;
		ctx->state = promise_state::no_callback;
		return future<T, Deteler>(*ctx);
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

	void set_value(T value, std::function<void(T)> undo = nullptr) {
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

	void set_value(std::function<void()> undo = nullptr) {
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
