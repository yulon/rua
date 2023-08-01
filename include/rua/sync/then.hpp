#ifndef _rua_sync_then_hpp
#define _rua_sync_then_hpp

#include "await.hpp"
#include "future.hpp"
#include "promise.hpp"

#include "../invocable.hpp"

#include <functional>

namespace rua {

template <
	typename Awaitable,
	typename Callback,
	typename ExpR = decltype(expected_invoke(
		std::declval<Callback &&>(),
		std::declval<
			warp_expected_t<decay_t<await_result_t<Awaitable &&>>>>())),
	typename R = unwarp_expected_t<ExpR>>
inline future<R> then(Awaitable &&awaitable, Callback &&callback) {
	auto aw = wrap_awaiter(std::forward<Awaitable>(awaitable));
	if (aw->await_ready()) {
		return expected_invoke(std::forward<Callback>(callback), [&]() {
			return aw->await_resume();
		});
	}

	struct ctx_t {
		awaiter_wrapper<Awaitable &&> aw;
		decay_t<Callback> cb;
	};

	auto prm = new newable_promise<R, ctx_t>(
		ctx_t{std::move(aw), std::forward<Callback>(callback)});

	if (await_suspend(*prm->extend().aw, [prm]() {
			prm->fulfill(expected_invoke(prm->extend().cb, [prm]() {
				return prm->extend().aw->await_resume();
			}));
		})) {
		return future<R>(*prm);
	}

	auto exp = expected_invoke(prm->extend().cb, [prm]() mutable {
		return prm->extend().aw->await_resume();
	});

	prm->unuse();

	return exp;
}

namespace await_operators {

template <typename Awaitable, typename Callback>
inline decltype(then(std::declval<Awaitable &&>(), std::declval<Callback &&>()))
operator>>(Awaitable &&awaitable, Callback &&callback) {
	return then(
		std::forward<Awaitable>(awaitable), std::forward<Callback>(callback));
}

} // namespace await_operators

} // namespace rua

#endif
