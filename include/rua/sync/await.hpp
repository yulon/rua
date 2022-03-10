#ifndef _RUA_SYNC_AWAIT_HPP
#define _RUA_SYNC_AWAIT_HPP

#include "../coroutine.hpp"
#include "../util.hpp"

#include <cassert>

namespace rua {

#ifdef RUA_AWAIT_SUPPORTED
#define RUA_OPERATOR_AWAIT operator co_await
#else
#define RUA_OPERATOR_AWAIT operator_co_await
#endif

////////////////////////////////////////////////////////////////////////////

template <typename, typename = void>
struct is_awaiter : std::false_type {};

template <typename T>
struct is_awaiter<
	T,
	void_t<
		decltype(std::declval<T>().await_ready()),
		decltype(std::declval<T>().await_suspend(
			std::declval<coroutine_handle<>>())),
		decltype(std::declval<T>().await_resume())>> : std::true_type {};

template <typename T, typename = void>
struct is_overloaded_await : std::false_type {};

template <typename T>
struct is_overloaded_await<
	T,
	void_t<decltype(std::declval<T>().RUA_OPERATOR_AWAIT())>> : std::true_type {
};

template <typename T>
struct is_awaitable
	: bool_constant<is_awaiter<T>::value || is_overloaded_await<T>::value> {};

template <typename T>
inline enable_if_t<is_awaiter<T &&>::value, T &&> make_awaiter(T &&awaiter) {
	return std::forward<T>(awaiter);
}

template <typename T>
inline enable_if_t<
	is_overloaded_await<T>::value,
	decltype(std::declval<T &&>().RUA_OPERATOR_AWAIT())>
make_awaiter(T &&awaitable) {
	return std::forward<T>(awaitable).RUA_OPERATOR_AWAIT();
}

template <typename T>
using awaiter = type_identity<decltype(make_awaiter(std::declval<T>()))>;

template <typename T>
using awaiter_t = typename awaiter<T>::type;

template <typename T>
using await_result =
	type_identity<decltype(make_awaiter(std::declval<T>()).await_resume())>;

template <typename T>
using await_result_t = typename await_result<T>::type;

////////////////////////////////////////////////////////////////////////////

template <
	typename Awaiter,
	typename Resume = coroutine_handle<>,
	typename Result =
		decltype(std::declval<Awaiter>().await_suspend(std::declval<Resume>()))>
struct strict_await_suspend_result : type_identity<Result> {};

template <typename Awaiter, typename Resume = coroutine_handle<>>
using strict_await_suspend_result_t =
	typename strict_await_suspend_result<Awaiter, Resume>::type;

template <typename Awaiter, typename Resume>
inline strict_await_suspend_result_t<Awaiter &&, Resume>
_await_suspend(Awaiter &&awaiter, Resume resume) {
	return std::forward<Awaiter>(awaiter).await_suspend(std::move(resume));
}

template <typename Awaiter, typename Resume, typename = void>
struct is_bad_await_suspend_paramenter : std::true_type {};

template <typename Awaiter, typename Resume>
struct is_bad_await_suspend_paramenter<
	Awaiter,
	Resume,
	void_t<strict_await_suspend_result<Awaiter, Resume>>> : std::false_type {};

#ifdef __cpp_lib_coroutine

template <typename Awaiter, typename Callback>
inline enable_if_t<
	is_bad_await_suspend_paramenter<Awaiter &&, Callback>::value &&
		std::is_void<invoke_result_t<Callback>>::value,
	strict_await_suspend_result<Awaiter &&>>
_await_suspend(Awaiter &&awaiter, Callback callback) {
	struct callback_coroutine {
		struct promise_type {
			constexpr promise_type() = default;

			callback_coroutine get_return_object() {
				return {coroutine_handle<promise_type>::from_promise(*this)};
			}

			suspend_always initial_suspend() {
				return {};
			}

			suspend_and_destroy final_suspend() noexcept {
				return {};
			}

			void return_void() {}

			void unhandled_exception() {}
		};
		coroutine_handle<promise_type> co;
	};
	auto cb = skate(callback);
	return std::forward<Awaiter>(awaiter).await_suspend(
		([cb]() -> callback_coroutine { co_return cb(); })().co);
}

#endif

template <
	typename Awaiter,
	typename Resume = coroutine_handle<>,
	typename Result = decltype(_await_suspend(
		std::declval<Awaiter>(), std::declval<Resume>()))>
struct await_suspend_result : type_identity<Result> {};

template <typename Awaiter, typename Resume = coroutine_handle<>>
using await_suspend_result_t =
	typename strict_await_suspend_result<Awaiter, Resume>::type;

template <
	typename Awaiter,
	typename Resume,
	typename Result = await_suspend_result_t<Awaiter &&, Resume>>
inline enable_if_t<!std::is_void<Result>::value, Result>
await_suspend(Awaiter &&awaiter, Resume resume) {
	return std::forward<Awaiter>(awaiter).await_suspend(std::move(resume));
}

template <
	typename Awaiter,
	typename Resume,
	typename Result = await_suspend_result_t<Awaiter &&, Resume>>
inline enable_if_t<std::is_void<Result>::value, bool>
await_suspend(Awaiter &&awaiter, Resume resume) {
	std::forward<Awaiter>(awaiter).await_suspend(std::move(resume));
	return true;
}

} // namespace rua

#endif
