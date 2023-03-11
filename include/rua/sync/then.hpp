#ifndef _RUA_SYNC_THEN_HPP
#define _RUA_SYNC_THEN_HPP

#include "await.hpp"
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
		return expected_invoke(
			std::forward<Callback>(callback), &awtr->await_resume, *awtr);
	}
	struct ctx_t {
		promise<R> prm;
		awaiter_wrapper<Awaitable &&> awtr;
		decay_t<Callback> cb;
	};
	auto ctx = new ctx_t{{}, std::move(awtr), std::forward<Callback>(callback)};
	auto r = ctx->prm.get_future();
	if (await_suspend(*ctx->awtr, [ctx]() {
			ctx->prm.deliver(
				expected_invoke(ctx->cb, &ctx->awtr->await_resume, *ctx->awtr));
			delete ctx;
		})) {
		return r;
	}
	r = expected_invoke(ctx->cb, &ctx->awtr->await_resume, *ctx->awtr);
	delete ctx;
	return r;
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
