#ifndef _RUA_SYNC_FUTURE_HPP
#define _RUA_SYNC_FUTURE_HPP

#include "wait.hpp"

#include "../async/result.hpp"
#include "../optional.hpp"
#include "../util.hpp"

#include <cassert>

namespace rua {

template <typename T = void, typename Promised = T>
class future : public waiter<future<T, Promised>, T> {
public:
	constexpr future() : _r(), _ar() {}

	template <
		typename... Result,
		typename = enable_if_t<
			std::is_constructible<T, Result &&...>::value &&
			(sizeof...(Result) > 1 ||
			 !std::is_base_of<future, decay_t<front_t<Result &&...>>>::value)>>
	future(Result &&...result) : _r(std::forward<Result>(result)...) {}

	explicit future(async_result<T> ar) : _ar(std::move(ar)) {}

	explicit future(async_result_context<T> &ctx) : _ar(ctx) {}

	template <
		typename U,
		typename = enable_if_t<
			std::is_constructible<T, U &&>::value &&
			!std::is_constructible<T, future<U, Promised> &&>::value>>
	future(future<U, Promised> &&src) :
		_r(std::move(src._r)), _ar(std::move(src._ar)) {}

	template <
		typename U,
		typename = enable_if_t<!std::is_constructible<T, U &&>::value>,
		typename = enable_if_t<
			!std::is_constructible<T, future<U, Promised> &&>::value>>
	future(future<U, Promised> &&src) : _ar(std::move(src._ar)) {
		assert(!src._r);
	}

	future(future &&src) : _r(std::move(src._r)), _ar(std::move(src._ar)) {}

	RUA_OVERLOAD_ASSIGNMENT_R(future);

	bool await_ready() {
		return _r.has_value();
	}

	template <typename Resume>
	bool await_suspend(Resume resume) {
		assert(_ar);
#ifdef NDEBUG
		return _ar.try_set_on_put(std::move(resume)) ==
			   async_result_state::has_on_put;
#else
		auto state = _ar.try_set_on_put(std::move(resume));
		if (state == async_result_state::has_on_put) {
			return true;
		}
		assert(state == async_result_state::has_value);
		return false;
#endif
	}

	T await_resume() {
		if (!_r) {
			assert(_ar);
#ifdef NDEBUG
			return static_cast<T>(*_ar.checkout());
#else
			auto r = _ar.checkout();
			assert(r);
			return static_cast<T>(*std::move(r));
#endif
		}
		return *std::move(_r);
	}

	template <typename U>
	future<U, Promised> to() {
		return std::move(*this);
	}

	void reset() {
		_r.reset();
		_ar.reset();
	}

private:
	optional<T> _r;
	async_result<Promised> _ar;
};

template <>
class future<void, void> : public waiter<future<>> {
public:
	constexpr future(std::nullptr_t = nullptr) : _ar() {}

	explicit future(async_result<> ar) : _ar(std::move(ar)) {}

	explicit future(async_result_context<> &ctx) : _ar(ctx) {}

	future(future &&src) : _ar(std::move(src._ar)) {}

	RUA_OVERLOAD_ASSIGNMENT_R(future);

	bool await_ready() {
		return !_ar;
	}

	template <typename Resume>
	bool await_suspend(Resume resume) {
		assert(_ar);
#ifdef NDEBUG
		return _ar.try_set_on_put(std::move(resume)) ==
			   async_result_state::has_on_put;
#else
		auto state = _ar.try_set_on_put(std::move(resume));
		if (state == async_result_state::has_on_put) {
			return true;
		}
		assert(state == async_result_state::has_value);
		return false;
#endif
	}

	void await_resume() const {}

	void reset() {
		_ar.reset();
	}

private:
	async_result<> _ar;
};

template <typename T>
class future<T, void> {};

template <typename T = void>
class promise {
public:
	constexpr promise() : _ar_put() {}

	explicit promise(async_result_putter<T> ar_put) :
		_ar_put(std::move(ar_put)) {}

	explicit promise(async_result_context<T> &ctx) : _ar_put(ctx) {}

	explicit operator bool() const {
		return !!_ar_put;
	}

	future<T> get_future() {
		async_result<T> ar;
		_ar_put = ar.get_putter();
		return future<T>(std::move(ar));
	}

	void resolve(T value, std::function<void(T)> undo = nullptr) {
		assert(_ar_put);
		_ar_put(std::move(value), std::move(undo));
	}

	async_result_putter<T> release() {
		return std::move(_ar_put);
	}

	void reset() {
		_ar_put.reset();
	}

private:
	async_result_putter<T> _ar_put;
};

template <>
class promise<void> {
public:
	constexpr promise() : _ar_put() {}

	explicit promise(async_result_putter<> ar_put) :
		_ar_put(std::move(ar_put)) {}

	explicit promise(async_result_context<> &ctx) : _ar_put(ctx) {}

	explicit operator bool() const {
		return !!_ar_put;
	}

	future<> get_future() {
		async_result<> ar;
		_ar_put = ar.get_putter();
		return future<>(std::move(ar));
	}

	void resolve(std::function<void()> undo = nullptr) {
		assert(_ar_put);
		_ar_put(std::move(undo));
	}

	async_result_putter<> release() {
		return exchange(_ar_put, {});
	}

	void reset() {
		_ar_put.reset();
	}

private:
	async_result_putter<> _ar_put;
};

} // namespace rua

#endif
