#ifndef _RUA_SCHED_WAIT_UNI_HPP
#define _RUA_SCHED_WAIT_UNI_HPP

#include "../scheduler.hpp"

#include "../../sched/scheduler.hpp"
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
wait(scheduler_i sch, Callee &&callee, Args &&... args) {
	assert(sch);

	auto ch = std::make_shared<chan<bool>>();
	pa([=]() mutable {
		callee(args...);
		*ch << true;
	});
	ch->pop(std::move(sch));
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
wait(scheduler_i sch, Callee &&callee, Args &&... args) {
	assert(sch);

	auto ch = std::make_shared<chan<Ret>>();
	pa([=]() mutable { *ch << callee(args...); });
	return ch->pop(std::move(sch));
}

template <typename Ret, typename... Args>
inline Ret wait(scheduler_i sch, Ret (&callee)(Args...), Args... args) {
	return wait(std::move(sch), &callee, std::move(args)...);
}

template <
	typename Callee,
	typename... Args,
	typename Ret =
		decltype(std::declval<Callee &&>()(std::declval<Args &&>()...))>
inline enable_if_t<!std::is_function<remove_reference_t<Callee>>::value, Ret>
wait(Callee &&callee, Args &&... args) {
	auto sch = this_scheduler();
	if (sch.type_is<thread_scheduler>()) {
		return std::forward<Callee>(callee)(std::forward<Args>(args)...);
	}
	return wait(
		std::move(sch),
		std::forward<Callee>(callee),
		std::forward<Args>(args)...);
}

template <typename Ret, typename... Args>
inline Ret wait(Ret (&callee)(Args...), Args... args) {
	return wait(&callee, std::move(args)...);
}

} // namespace rua

#endif
