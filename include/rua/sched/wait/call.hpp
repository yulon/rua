#ifndef _RUA_SCHED_WAIT_CALL_HPP
#define _RUA_SCHED_WAIT_CALL_HPP

#include "../../thread/pa.hpp"
#include "../util.hpp"

#include <utility>

namespace rua {

template <
	typename Callee,
	typename... Args,
	typename Ret =
		decltype(std::declval<Callee &>()(std::declval<Args &>()...))>
inline enable_if_t<
	!std::is_function<remove_reference_t<Callee>>::value &&
		std::is_same<Ret, void>::value,
	Ret>
wait(Callee &&callee, Args &&... args) {
	auto sch = this_scheduler();
	if (sch.type_is<thread_scheduler>()) {
		callee(std::forward<Args>(args)...);
		return;
	}
	auto sig = sch->get_signaler();
	pa([=]() mutable {
		callee(args...);
		sig->signal();
	});
	sch->wait();
}

template <
	typename Callee,
	typename... Args,
	typename Ret =
		decltype(std::declval<Callee &>()(std::declval<Args &>()...))>
enable_if_t<
	!std::is_function<remove_reference_t<Callee>>::value &&
		!std::is_same<Ret, void>::value,
	Ret>
wait(Callee &&callee, Args &&... args) {
	auto sch = this_scheduler();
	if (sch.type_is<thread_scheduler>()) {
		return callee(std::forward<Args>(args)...);
	}
	auto sig = sch->get_signaler();
	auto r_ptr = new Ret;
	pa([=]() mutable {
		*r_ptr = callee(args...);
		sig->signal();
	});
	sch->wait();
	auto r = *r_ptr;
	delete r_ptr;
	return r;
}

template <typename Ret, typename... Params, typename... Args>
Ret wait(Ret (&callee)(Params...), Args &&... args) {
	return wait(&callee, std::forward<Args>(args)...);
}

} // namespace rua

#endif
