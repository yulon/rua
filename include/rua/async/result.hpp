#ifndef _RUA_ASYNC_RESULT_HPP
#define _RUA_ASYNC_RESULT_HPP

#include "../optional.hpp"
#include "../skater.hpp"
#include "../util.hpp"

#include <atomic>
#include <cassert>
#include <functional>

namespace rua {

enum class async_result_state : uintptr_t {
	no_on_put,
	has_on_put,
	has_value,
	done
};

template <typename T>
struct async_result_context_base {
	std::atomic<async_result_state> state;
	std::function<void()> on_put;
};

template <typename T = void>
struct async_result_context : async_result_context_base<T> {
	optional<T> value;
	std::function<void(T)> undo;
};

template <>
struct async_result_context<void> : async_result_context_base<void> {
	std::function<void()> undo;
};

////////////////////////////////////////////////////////////////////////////

template <typename T>
class async_result_putter_base {
public:
	constexpr async_result_putter_base() : _ctx(nullptr) {}

	explicit async_result_putter_base(async_result_context<T> &ctx) :
		_ctx(&ctx) {}

	~async_result_putter_base() {
		reset();
	}

	async_result_putter_base(async_result_putter_base &&src) :
		_ctx(exchange(src._ctx, nullptr)) {}

	RUA_OVERLOAD_ASSIGNMENT_R(async_result_putter_base)

	async_result_context<T> *context() const {
		return _ctx;
	}

	explicit operator bool() const {
		return _ctx;
	}

	void reset() {
		if (!_ctx) {
			return;
		}

		auto old_state = _ctx->state.exchange(async_result_state::done);
		assert(old_state != async_result_state::has_value);

		switch (old_state) {
		case async_result_state::has_on_put:
			_ctx->on_put = nullptr;
			break;

		case async_result_state::done:
			delete _ctx;
			break;

		default:
			break;
		}

		_ctx = nullptr;
	}

	async_result_context<T> *release() {
		return exchange(_ctx, nullptr);
	}

protected:
	async_result_context<T> *_ctx;
};

template <typename T = void>
class async_result_putter : public async_result_putter_base<T> {
public:
	constexpr async_result_putter() = default;

	explicit async_result_putter(async_result_context<T> &ctx) :
		async_result_putter_base<T>(ctx) {}

	void operator()(T value, std::function<void(T)> undo = nullptr) {
		auto &ctx = this->_ctx;
		assert(ctx);

		assert(!ctx->value);
		ctx->value.emplace(std::move(value));

		if (undo) {
			assert(!ctx->undo);
			ctx->undo = std::move(undo);
		}

		auto old_state = ctx->state.exchange(async_result_state::has_value);
		assert(old_state != async_result_state::has_value);

		switch (old_state) {
		case async_result_state::has_on_put: {
			assert(ctx->on_put);
			auto on_put = std::move(ctx->on_put);
			on_put();
			break;
		}

		case async_result_state::done:
			if (ctx->undo) {
				ctx->undo(*std::move(ctx->value));
			};
			delete ctx;
			break;

		default:
			break;
		}

		ctx = nullptr;
	}
};

template <>
class async_result_putter<void> : public async_result_putter_base<void> {
public:
	constexpr async_result_putter() = default;

	explicit async_result_putter(async_result_context<void> &ctx) :
		async_result_putter_base<void>(ctx) {}

	void operator()(std::function<void()> undo = nullptr) {
		auto &ctx = this->_ctx;
		assert(ctx);

		if (undo) {
			assert(!ctx->undo);
			ctx->undo = std::move(undo);
		}

		auto old_state = ctx->state.exchange(async_result_state::has_value);
		assert(old_state != async_result_state::has_value);

		switch (old_state) {
		case async_result_state::has_on_put:
			assert(ctx->on_put);
			ctx->on_put();
			break;

		case async_result_state::done:
			if (ctx->undo) {
				ctx->undo();
			};
			delete ctx;
			break;

		default:
			break;
		}

		ctx = nullptr;
	}
};

////////////////////////////////////////////////////////////////////////////

template <typename T>
class async_result_base {
public:
	~async_result_base() {
		reset();
	}

	async_result_base(async_result_base &&src) :
		_ctx(exchange(src._ctx, nullptr)) {}

	RUA_OVERLOAD_ASSIGNMENT_R(async_result_base)

	async_result_context<T> *context() const {
		return _ctx;
	}

	explicit operator bool() const {
		return _ctx;
	}

	async_result_state try_set_on_put(std::function<void()> on_put) {
		if (!_ctx) {
			_ctx = new async_result_context<T>;
			_ctx->on_put = std::move(on_put);
			_ctx->state = async_result_state::has_on_put;
			return async_result_state::has_on_put;
		}

		_ctx->on_put = std::move(on_put);

		auto old_state = _ctx->state.load();
		while (old_state == async_result_state::no_on_put &&
			   !_ctx->state.compare_exchange_weak(
				   old_state, async_result_state::has_on_put))
			;
		assert(old_state != async_result_state::has_on_put);

		switch (old_state) {
		case async_result_state::no_on_put:
			return async_result_state::has_on_put;

		case async_result_state::done:
			delete _ctx;
			_ctx = nullptr;
			break;

		default:
			break;
		}

		return old_state;
	}

	async_result_putter<T> get_putter(std::function<void()> on_put) {
		assert(!_ctx);
#ifdef NDEBUG
		try_set_on_put(std::move(on_put));
#else
		assert(
			try_set_on_put(std::move(on_put)) ==
			async_result_state::has_on_put);
#endif
		return async_result_putter<T>(*_ctx);
	}

	async_result_putter<T> get_putter() {
		if (!_ctx) {
			_ctx = new async_result_context<T>;
			_ctx->state = async_result_state::no_on_put;
		}
		return async_result_putter<T>(*_ctx);
	}

	void reset() {
		if (!_ctx) {
			return;
		}

		auto old_state = _ctx->state.exchange(async_result_state::done);

		switch (old_state) {
		case async_result_state::has_value:
			_back_to_putter();
			RUA_FALLTHROUGH;

		case async_result_state::done:
			delete _ctx;
			break;

		default:
			break;
		}

		_ctx = nullptr;
	}

	async_result_context<T> *release() {
		return exchange(_ctx, nullptr);
	}

protected:
	async_result_context<T> *_ctx;

	constexpr async_result_base() : _ctx(nullptr) {}

	explicit async_result_base(async_result_context<T> &ctx) : _ctx(&ctx) {}

	inline void _back_to_putter();
};

template <typename T>
inline void async_result_base<T>::_back_to_putter() {
	assert(_ctx);
	assert(_ctx->value);

	if (!_ctx->undo) {
		_ctx->value.reset();
		return;
	}
	_ctx->undo(*std::move(_ctx->value));
}

template <>
inline void async_result_base<void>::_back_to_putter() {
	assert(_ctx);

	if (!_ctx->undo) {
		return;
	}
	_ctx->undo();
}

template <typename T = void>
class async_result : public async_result_base<T> {
public:
	constexpr async_result() : async_result_base<T>() {}

	explicit async_result(async_result_context<T> &ctx) :
		async_result_base<T>(ctx) {}

	optional<T> checkout() {
		optional<T> r;

		auto &ctx = this->_ctx;

		if (!ctx) {
			return r;
		}

		auto old_state = ctx->state.exchange(async_result_state::done);

		switch (old_state) {

		case async_result_state::has_value:
			r = std::move(ctx->value);
			RUA_FALLTHROUGH;

		case async_result_state::done:
			delete ctx;
			break;

		default:
			break;
		}

		ctx = nullptr;
		return r;
	}

	bool checkout_or_lose_putter(bool lose_when_putting) {
		if (lose_when_putting) {
			return checkout();
		}
		auto ctx = this->_ctx;
		auto r = checkout();
		if (!r) {
			delete ctx;
		}
		return r;
	}
};

template <>
class async_result<void> : public async_result_base<void> {
public:
	constexpr async_result() : async_result_base<void>() {}

	explicit async_result(async_result_context<void> &ctx) :
		async_result_base<void>(ctx) {}

	bool checkout() {
		auto &ctx = this->_ctx;
		assert(ctx);

		if (!ctx) {
			return false;
		}

		auto old_state = _ctx->state.exchange(async_result_state::done);

		switch (old_state) {

		case async_result_state::has_value:
			return true;

		case async_result_state::done:
			delete ctx;
			break;

		default:
			break;
		}

		ctx = nullptr;
		return false;
	}

	bool checkout_or_lose_putter(bool lose_when_putting) {
		if (lose_when_putting) {
			return checkout();
		}
		auto ctx = this->_ctx;
		auto r = checkout();
		if (!r) {
			delete ctx;
		}
		return r;
	}
};

} // namespace rua

#endif
