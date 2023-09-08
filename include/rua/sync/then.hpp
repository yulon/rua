#ifndef _rua_sync_then_hpp
#define _rua_sync_then_hpp

#include "await.hpp"
#include "future.hpp"
#include "promise.hpp"

#include "../invocable.hpp"

#include <functional>

namespace rua {

template <typename R, typename Awaitable, typename Callback>
inline enable_if_t<!is_awaitable<R>::value, future<R>>
_then(Awaitable &&awaitable, Callback &&callback) {
	auto aw = wrap_awaiter(std::forward<Awaitable>(awaitable));
	if (aw->await_ready()) {
		return try_invoke(std::forward<Callback>(callback), [&]() {
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
			prm->fulfill(try_invoke(prm->extend().cb, [prm]() {
				return prm->extend().aw->await_resume();
			}));
		})) {
		return future<R>(*prm);
	}

	auto r_exp = try_invoke(
		prm->extend().cb, [prm]() { return prm->extend().aw->await_resume(); });

	prm->unuse();

	return r_exp;
}

template <
	typename Awaitable,
	typename Callback,
	typename ExpR = decltype(try_invoke(
		std::declval<Callback &&>(),
		std::declval<
			warp_expected_t<decay_t<await_result_t<Awaitable &&>>>>())),
	typename R = unwarp_expected_t<ExpR>>
inline decltype(_then<R>(
	std::declval<Awaitable &&>(), std::declval<Callback &&>()))
then(Awaitable &&awaitable, Callback &&callback) {
	return _then<R>(
		std::forward<Awaitable>(awaitable), std::forward<Callback>(callback));
}

template <
	typename R,
	typename Awaitable,
	typename Callback,
	typename R2 = unwarp_expected_t<decay_t<await_result_t<R>>>>
inline enable_if_t<is_awaitable<R>::value, future<R2>>
_then(Awaitable &&awaitable, Callback &&callback) {
	auto aw = wrap_awaiter(std::forward<Awaitable>(awaitable));
	if (aw->await_ready()) {
		auto r_exp = try_invoke(std::forward<Callback>(callback), [&]() {
			return aw->await_resume();
		});

		if (!r_exp) {
			return r_exp.error();
		}

		return _then<R2>(*std::move(r_exp), [](expected<R2> r2_exp) {
			return std::move(r2_exp);
		});
	}

	struct ctx_t {
		awaiter_wrapper<Awaitable &&> aw;
		decay_t<Callback> cb;
	};

	auto prm = new newable_promise<R2, ctx_t>(
		ctx_t{std::move(aw), std::forward<Callback>(callback)});

	if (await_suspend(*prm->extend().aw, [prm]() {
			auto r_exp = try_invoke(prm->extend().cb, [prm]() {
				return prm->extend().aw->await_resume();
			});
			if (!r_exp) {
				prm->fulfill(r_exp.error());
				return;
			}
			then(*std::move(r_exp), [prm](expected<R2> r2_exp) {
				prm->fulfill(std::move(r2_exp));
			});
		})) {
		return future<R2>(*prm);
	}

	auto r_exp = try_invoke(
		prm->extend().cb, [prm]() { return prm->extend().aw->await_resume(); });

	prm->unuse();

	if (!r_exp) {
		return r_exp.error();
	}

	return _then<R2>(*std::move(r_exp), [](expected<R2> r2_exp) {
		return std::move(r2_exp);
	});
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
