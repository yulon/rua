#ifndef _RUA_THREAD_AWAIT_HPP
#define _RUA_THREAD_AWAIT_HPP

#include "suspender.hpp"

#include "../sync/chan.hpp"
#include "../thread/reuse.hpp"
#include "../types/util.hpp"

#include <memory>

namespace rua {

template <
	typename F,
	typename... Args,
	typename Ret = invoke_result_t<F &&, Args &&...>>
inline enable_if_t<std::is_same<Ret, void>::value>
await(suspender_i spdr, F &&f, Args &&...args) {
	assert(spdr);

	if (spdr.type_is<thread_suspender>()) {
		return std::forward<F>(f)(std::forward<Args>(args)...);
	}
	auto ch_ptr = new chan<bool>;
	std::unique_ptr<chan<bool>> ch_uptr(ch_ptr);
	reuse_thread([=]() mutable {
		f(args...);
		*ch_ptr << true;
	});
	ch_ptr->pop(std::move(spdr));
}

template <
	typename F,
	typename... Args,
	typename Ret = invoke_result_t<F &&, Args &&...>>
inline enable_if_t<!std::is_same<Ret, void>::value, Ret>
await(suspender_i spdr, F &&f, Args &&...args) {
	assert(spdr);

	if (spdr.type_is<thread_suspender>()) {
		return std::forward<F>(f)(std::forward<Args>(args)...);
	}
	auto ch_ptr = new chan<Ret>;
	std::unique_ptr<chan<Ret>> ch_uptr(ch_ptr);
	reuse_thread([=]() mutable { *ch_ptr << f(args...); });
	return ch_ptr->pop(std::move(spdr));
}

template <typename F, typename... Args>
inline invoke_result_t<F &&, Args &&...> await(F &&f, Args &&...args) {
	return await(
		this_suspender(), std::forward<F>(f), std::forward<Args>(args)...);
}

} // namespace rua

#endif
