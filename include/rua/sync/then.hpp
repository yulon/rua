#ifndef _RUA_SYNC_THEN_HPP
#define _RUA_SYNC_THEN_HPP

#include "await.hpp"
#include "future.hpp"
#include "promise.hpp"

#include "../invocable.hpp"
#include "../skater.hpp"

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
	auto awtr = wrap_awaiter(std::forward<Awaitable>(awaitable));
	if (awtr->await_ready()) {
		return expected_invoke(std::forward<Callback>(callback), [&]() {
			return awtr->await_resume();
		});
	}

	class then_promise : public newable_promise<R> {
	public:
		virtual ~then_promise() {}

		awaiter_wrapper<Awaitable &&> then_awtr;
		decay_t<Callback> then_cb;

		then_promise(awaiter_wrapper<Awaitable &&> awtr, decay_t<Callback> cb) :
			newable_promise<R>(),
			then_awtr(std::move(awtr)),
			then_cb(std::move(cb)) {}
	};

	auto prm =
		new then_promise(std::move(awtr), std::forward<Callback>(callback));

	if (await_suspend(*prm->then_awtr, [prm]() {
			prm->deliver(expected_invoke(prm->then_cb, [prm]() {
				return prm->then_awtr->await_resume();
			}));
		})) {
		return future<R>(*prm); // automatically delete promise.
	}

	auto exp = expected_invoke(prm->then_cb, [prm]() mutable {
		return prm->then_awtr->await_resume();
	});

	delete prm; // useless, delete promise manually.

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
