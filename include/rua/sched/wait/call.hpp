#ifndef _RUA_SCHED_WAIT_CALL_HPP
#define _RUA_SCHED_WAIT_CALL_HPP

#include "../scheduler.hpp"

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
wait(Callee &&callee, Args &&... args) {
	auto sch = this_scheduler();
	if (sch.type_is<thread_scheduler>()) {
		std::forward<Callee>(callee)(std::forward<Args>(args)...);
		return;
	}
	auto ch = std::make_shared<chan<bool>>();
	pa([=]() mutable {
		callee(args...);
		*ch << true;
	});
	ch->pop(sch);
}

template <
	typename Callee,
	typename... Args,
	typename Ret =
		decltype(std::declval<Callee &&>()(std::declval<Args &&>()...))>
enable_if_t<
	!std::is_function<remove_reference_t<Callee>>::value &&
		!std::is_same<Ret, void>::value,
	Ret>
wait(Callee &&callee, Args &&... args) {
	auto sch = this_scheduler();
	if (sch.type_is<thread_scheduler>()) {
		return std::forward<Callee>(callee)(std::forward<Args>(args)...);
	}
	auto ch = std::make_shared<chan<Ret>>();
	pa([=]() mutable { *ch << callee(args...); });
	return ch->pop(sch);
}

template <typename Ret, typename... Params, typename... Args>
Ret wait(Ret (&callee)(Params...), Args &&... args) {
	return wait(&callee, std::forward<Args>(args)...);
}

} // namespace rua

#endif
