#ifndef _RUA_SCHED_WAIT_UNI_HPP
#define _RUA_SCHED_WAIT_UNI_HPP

#include "../suspender.hpp"

#include "../../sched/suspender.hpp"
#include "../../sync/chan.hpp"
#include "../../thread/pa.hpp"
#include "../../types/util.hpp"

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
wait(suspender_i spdr, Callee &&callee, Args &&...args) {
	assert(spdr);

	if (spdr.type_is<thread_suspender>()) {
		return std::forward<Callee>(callee)(std::forward<Args>(args)...);
	}
	auto ch = std::make_shared<chan<bool>>();
	pa([=]() mutable {
		callee(args...);
		*ch << true;
	});
	ch->pop(std::move(spdr));
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
wait(suspender_i spdr, Callee &&callee, Args &&...args) {
	assert(spdr);

	if (spdr.type_is<thread_suspender>()) {
		return std::forward<Callee>(callee)(std::forward<Args>(args)...);
	}
	auto ch = std::make_shared<chan<Ret>>();
	pa([=]() mutable { *ch << callee(args...); });
	return ch->pop(std::move(spdr));
}

template <typename Ret, typename... Args>
inline Ret wait(suspender_i spdr, Ret (&callee)(Args...), Args... args) {
	return wait(std::move(spdr), &callee, std::move(args)...);
}

template <
	typename Callee,
	typename... Args,
	typename Ret =
		decltype(std::declval<Callee &&>()(std::declval<Args &&>()...))>
inline enable_if_t<!std::is_function<remove_reference_t<Callee>>::value, Ret>
wait(Callee &&callee, Args &&...args) {
	return wait(
		this_suspender(),
		std::forward<Callee>(callee),
		std::forward<Args>(args)...);
}

template <typename Ret, typename... Args>
inline Ret wait(Ret (&callee)(Args...), Args... args) {
	return wait(&callee, std::move(args)...);
}

} // namespace rua

#endif
