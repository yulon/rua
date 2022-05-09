#ifndef _RUA_SYNC_LATE_HPP
#define _RUA_SYNC_LATE_HPP

#include "promise.hpp"

#include "../util.hpp"
#include "../variant.hpp"

#include <cassert>

namespace rua {

template <
	typename T = void,
	typename Promised = T,
	typename PromiseDeteler = default_promise_deleter<T>>
class late : private enable_wait_operator {
public:
	constexpr late() : _r() {}

	template <
		typename... Result,
		typename Front = decay_t<front_t<Result &&...>>,
		typename = enable_if_t<
			std::is_constructible<T, Result &&...>::value &&
			(sizeof...(Result) > 1 ||
			 (!std::is_base_of<late, Front>::value &&
			  !std::is_base_of<future<Promised, PromiseDeteler>, Front>::
				  value))>>
	late(Result &&...result) : _r(std::forward<Result>(result)...) {}

	late(future<Promised, PromiseDeteler> fut) : _r(std::move(fut)) {}

	explicit late(promise_context<Promised> &prm_ctx) :
		_r(future<Promised, PromiseDeteler>(prm_ctx)) {}

	template <
		typename U,
		typename = enable_if_t<
			!std::is_same<T, U>::value && std::is_convertible<U &&, T>::value &&
			!std::is_convertible<late<U, Promised> &&, T>::value>>
	late(late<U, Promised, PromiseDeteler> &&src) : _r(std::move(src._r)) {}

	template <
		typename U,
		typename = enable_if_t<!std::is_convertible<U &&, T>::value>,
		typename =
			enable_if_t<!std::is_convertible<late<U, Promised> &&, T>::value>>
	late(late<U, Promised, PromiseDeteler> &&src) :
		_r(std::move(src._r).template as<future<Promised, PromiseDeteler>>()) {}

	late(late &&src) : _r(std::move(src._r)) {}

	RUA_OVERLOAD_ASSIGNMENT_R(late);

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
	late<U, Promised> to() {
		return std::move(*this);
	}

	void reset() {
		_r.reset();
	}

private:
	variant<T, future<Promised, PromiseDeteler>> _r;
};

template <typename PromiseDeteler>
class late<void, void, PromiseDeteler> : private enable_wait_operator {
public:
	constexpr late(std::nullptr_t = nullptr) : _fut() {}

	late(future<void, PromiseDeteler> fut) : _fut(std::move(fut)) {}

	explicit late(promise_context<> &prm_ctx) : _fut(prm_ctx) {}

	late(late &&src) : _fut(std::move(src._fut)) {}

	RUA_OVERLOAD_ASSIGNMENT_R(late);

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
class late<T, void, PromiseDeteler> {};

} // namespace rua

#endif
