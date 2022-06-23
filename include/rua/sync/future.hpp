#ifndef _RUA_SYNC_FUTURE_HPP
#define _RUA_SYNC_FUTURE_HPP

#include "promise.hpp"

#include "../util.hpp"
#include "../variant.hpp"

#include <cassert>

namespace rua {

template <
	typename T = void,
	typename PromiseResult = T,
	typename PromiseDeteler = default_promise_deleter<T>>
class future : private enable_wait_operator {
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

	future(promise<PromiseResult, PromiseDeteler> &pms) :
		_r(pms.get_promising_future()) {}

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
class future<void, void, PromiseDeteler> : private enable_wait_operator {
public:
	constexpr future(std::nullptr_t = nullptr) : _pms_fut() {}

	future(promising_future<void, PromiseDeteler> pms_fut) :
		_pms_fut(std::move(pms_fut)) {}

	future(promise<void, PromiseDeteler> &pms) :
		_pms_fut(pms.get_promising_future()) {}

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

} // namespace rua

#endif
