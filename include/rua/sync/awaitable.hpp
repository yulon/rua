#ifndef _RUA_SYNC_AWAITABLE_HPP
#define _RUA_SYNC_AWAITABLE_HPP

#include "promise.hpp"

#include "../util.hpp"
#include "../variant.hpp"

#include <cassert>

namespace rua {

template <
	typename T = void,
	typename Promised = T,
	typename PromiseDeteler = default_promise_deleter<T>>
class awaitable : public waiter<awaitable<T, Promised>, T> {
public:
	constexpr awaitable() : _r() {}

	template <
		typename... Result,
		typename Front = decay_t<front_t<Result &&...>>,
		typename = enable_if_t<
			std::is_constructible<T, Result &&...>::value &&
			(sizeof...(Result) > 1 ||
			 (!std::is_base_of<awaitable, Front>::value &&
			  !std::is_base_of<future<Promised, PromiseDeteler>, Front>::
				  value))>>
	awaitable(Result &&...result) : _r(std::forward<Result>(result)...) {}

	awaitable(future<Promised, PromiseDeteler> fut) : _r(std::move(fut)) {}

	explicit awaitable(promise_context<Promised> &prm_ctx) :
		_r(future<Promised, PromiseDeteler>(prm_ctx)) {}

	template <
		typename U,
		typename = enable_if_t<
			!std::is_same<T, U>::value && std::is_convertible<U &&, T>::value &&
			!std::is_convertible<awaitable<U, Promised> &&, T>::value>>
	awaitable(awaitable<U, Promised, PromiseDeteler> &&src) :
		_r(std::move(src._r)) {}

	template <
		typename U,
		typename = enable_if_t<!std::is_convertible<U &&, T>::value>,
		typename = enable_if_t<
			!std::is_convertible<awaitable<U, Promised> &&, T>::value>>
	awaitable(awaitable<U, Promised, PromiseDeteler> &&src) :
		_r(std::move(src._r).template as<future<Promised, PromiseDeteler>>()) {}

	awaitable(awaitable &&src) : _r(std::move(src._r)) {}

	RUA_OVERLOAD_ASSIGNMENT_R(awaitable);

	bool await_ready() const {
		return _r.template type_is<T>();
	}

	template <typename Resume>
	bool await_suspend(Resume resume) {
		assert((_r.template type_is<future<Promised, PromiseDeteler>>()));

		return _r.template as<future<Promised, PromiseDeteler>>().await_suspend(
			std::move(resume));
	}

	T await_resume() {
		if (_r.template type_is<future<Promised, PromiseDeteler>>()) {
			auto &fut = _r.template as<future<Promised, PromiseDeteler>>();
			return static_cast<T>(fut.await_resume());
		}
		return std::move(_r).template as<T>();
	}

	template <typename U>
	awaitable<U, Promised> to() {
		return std::move(*this);
	}

	void reset() {
		_r.reset();
	}

private:
	variant<T, future<Promised, PromiseDeteler>> _r;
};

template <typename PromiseDeteler>
class awaitable<void, void, PromiseDeteler> : public waiter<awaitable<>> {
public:
	constexpr awaitable(std::nullptr_t = nullptr) : _fut() {}

	awaitable(future<void, PromiseDeteler> fut) : _fut(std::move(fut)) {}

	explicit awaitable(promise_context<> &prm_ctx) : _fut(prm_ctx) {}

	awaitable(awaitable &&src) : _fut(std::move(src._fut)) {}

	RUA_OVERLOAD_ASSIGNMENT_R(awaitable);

	bool await_ready() const {
		return !_fut;
	}

	template <typename Resume>
	bool await_suspend(Resume resume) {
		assert(_fut);

		return _fut.await_suspend(std::move(resume));
	}

	void await_resume() const {}

	void reset() {
		_fut.reset();
	}

private:
	future<void, PromiseDeteler> _fut;
};

template <typename T, typename PromiseDeteler>
class awaitable<T, void, PromiseDeteler> {};

} // namespace rua

#endif
