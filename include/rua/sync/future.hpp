#ifndef _RUA_SYNC_FUTURE_HPP
#define _RUA_SYNC_FUTURE_HPP

#include "await.hpp"
#include "promise.hpp"

#include "../error.hpp"
#include "../util.hpp"
#include "../variant.hpp"

#include <atomic>
#include <cassert>
#include <functional>
#include <memory>

namespace rua {

template <typename T>
class _future_detail {
public:
	template <typename PromisePtr>
	static inline expected<T> get_val(variant<T, error_i, PromisePtr> &v) {
		if (v.template type_is<T>()) {
			return std::move(v).template as<T>();
		}
		if (v.template type_is<error_i>()) {
			return std::move(v).template as<error_i>();
		}
		if (v.template type_is<PromisePtr>()) {
			auto prm_ptr = v.template as<PromisePtr>();
			v.reset();
			return prm_ptr->await_resume();
		}
		return unexpected;
	}
};

template <>
template <typename PromisePtr>
inline expected<void>
_future_detail<void>::get_val(variant<void, error_i, PromisePtr> &v) {
	if (v.template type_is<void>()) {
		return expected<void>();
	}
	if (v.template type_is<error_i>()) {
		return std::move(v).template as<error_i>();
	}
	if (v.template type_is<PromisePtr>()) {
		auto prm_ptr = v.template as<PromisePtr>();
		v.reset();
		return prm_ptr->await_resume();
	}
	return unexpected;
}

template <typename T = void, typename PromiseValue = T>
class future : private enable_await_operators {
public:
	constexpr future() : _r() {}

	RUA_CONSTRUCTIBLE_CONCEPT(Args, RUA_ARG(variant<T, error_i>), future)
	future(Args &&...args) : _r(std::forward<Args>(args)...) {}

	template <
		typename U = T,
		typename = enable_if_t<
			(std::is_void<T>::value && std::is_void<U>::value) ||
			std::is_convertible<U, T>::value>>
	future(expected<U> exp) : _r(std::move(exp).data()) {}

	template <
		typename Promise,
		typename = enable_if_t<
			std::is_convertible<Promise &, promise<PromiseValue> &>::value>>
	future(Promise &prm) : _r(&static_cast<promise<PromiseValue> &>(prm)) {}

	template <
		typename U,
		typename = enable_if_t<
			!std::is_same<T, U>::value && std::is_convertible<U &&, T>::value &&
			!std::is_convertible<future<U, PromiseValue> &&, T>::value>>
	future(future<U, PromiseValue> &&src) : _r(std::move(src._r)) {}

	template <
		typename U,
		typename = enable_if_t<!std::is_convertible<U &&, T>::value>,
		typename = enable_if_t<
			!std::is_convertible<future<U, PromiseValue> &&, T>::value>>
	future(future<U, PromiseValue> &&src) :
		_r(*std::move(src._r).template as<promise<PromiseValue> *>()) {}

	future(future &&src) : _r(std::move(src._r)) {}

	RUA_OVERLOAD_ASSIGNMENT(future);

	bool await_ready() const {
		return !_r.template type_is<promise<PromiseValue> *>();
	}

	bool await_suspend(std::function<void()> notify) {
		assert(!await_ready());

		return _r.template as<promise<PromiseValue> *>()->await_suspend(
			std::move(notify));
	}

	expected<T> await_resume() {
		return _future_detail<T>::get_val(_r);
	}

	template <typename U>
	future<U, PromiseValue> to() {
		return std::move(*this);
	}

	void reset() {
		if (_r.template type_is<promise<PromiseValue> *>()) {
			_r.template as<promise<PromiseValue> *>()->await_resume();
		}
		_r.reset();
	}

private:
	variant<T, error_i, promise<PromiseValue> *> _r;
};

} // namespace rua

#endif
