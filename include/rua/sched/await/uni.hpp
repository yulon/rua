#ifndef _RUA_SCHED_AWAIT_UNI_HPP
#define _RUA_SCHED_AWAIT_UNI_HPP

#include "../async/uni.hpp"
#include "../suspender.hpp"

#include "../../sched/suspender.hpp"
#include "../../sync/chan.hpp"
#include "../../types/util.hpp"

#include <memory>

namespace rua {

template <
	typename Callee,
	typename... Args,
	typename Ret =
		decltype(std::declval<Callee &&>()(std::declval<Args &&>()...))>
inline enable_if_t<
	!std::is_function<remove_reference_t<Callee>>::value &&
		std::is_same<Ret, void>::value,
	Ret>
await(suspender_i spdr, Callee &&callee, Args &&...args) {
	assert(spdr);

	if (spdr.type_is<thread_suspender>()) {
		return std::forward<Callee>(callee)(std::forward<Args>(args)...);
	}
	auto ch_ptr = new chan<bool>;
	std::unique_ptr<chan<bool>> ch_uptr(ch_ptr);
	async([=]() mutable {
		callee(args...);
		*ch_ptr << true;
	});
	ch_ptr->pop(std::move(spdr));
}

template <
	typename Callee,
	typename... Args,
	typename Ret =
		decltype(std::declval<Callee &&>()(std::declval<Args &&>()...))>
inline enable_if_t<
	!std::is_function<remove_reference_t<Callee>>::value &&
		!std::is_same<Ret, void>::value,
	Ret>
await(suspender_i spdr, Callee &&callee, Args &&...args) {
	assert(spdr);

	if (spdr.type_is<thread_suspender>()) {
		return std::forward<Callee>(callee)(std::forward<Args>(args)...);
	}
	auto ch_ptr = new chan<Ret>;
	std::unique_ptr<chan<Ret>> ch_uptr(ch_ptr);
	async([=]() mutable { *ch_ptr << callee(args...); });
	return ch_ptr->pop(std::move(spdr));
}

template <typename Ret, typename... Args>
inline Ret await(suspender_i spdr, Ret (&callee)(Args...), Args... args) {
	return await(std::move(spdr), &callee, std::move(args)...);
}

template <
	typename Callee,
	typename... Args,
	typename Ret =
		decltype(std::declval<Callee &&>()(std::declval<Args &&>()...))>
inline enable_if_t<!std::is_function<remove_reference_t<Callee>>::value, Ret>
await(Callee &&callee, Args &&...args) {
	return await(
		this_suspender(),
		std::forward<Callee>(callee),
		std::forward<Args>(args)...);
}

template <typename Ret, typename... Args>
inline Ret await(Ret (&callee)(Args...), Args... args) {
	return await(&callee, std::move(args)...);
}

} // namespace rua

#endif
