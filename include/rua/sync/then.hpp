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
	typename AwaitableResult = await_result_t<Awaitable &&>,
	typename CallbackResult = invoke_result_t<Callback &&, AwaitableResult>>
inline enable_if_t<
	!std::is_void<AwaitableResult>::value &&
		!std::is_void<CallbackResult>::value &&
		!is_awaitable<CallbackResult>::value,
	future<CallbackResult>>
then(Awaitable &&awaitable, Callback &&callback) {
	auto awtr = wrap_awaiter(std::forward<Awaitable>(awaitable));
	if (awtr->await_ready()) {
		return std::forward<Callback>(callback)(awtr->await_resume());
	}
	struct ctx_t {
		promise<CallbackResult> prm;
		awaiter_wrapper<Awaitable &&> awtr;
		std::function<CallbackResult(AwaitableResult)> cb;
	};
	auto ctx = new ctx_t{{}, std::move(awtr), std::forward<Callback>(callback)};
	auto r = ctx->prm.get_future();
	if (await_suspend(*ctx->awtr, [ctx]() {
			ctx->prm.deliver(ctx->cb(ctx->awtr->await_resume()));
			delete ctx;
		})) {
		return r;
	}
	r = ctx->cb(ctx->awtr->await_resume());
	delete ctx;
	return r;
}

template <
	typename Awaitable,
	typename Callback,
	typename AwaitableResult = await_result_t<Awaitable &&>,
	typename CallbackResult = invoke_result_t<Callback &&, AwaitableResult>,
	typename CallbackResultValue = await_result_t<CallbackResult>,
	typename Result = future<CallbackResultValue>>
inline enable_if_t<
	!std::is_void<AwaitableResult>::value &&
		!std::is_void<CallbackResult>::value &&
		std::is_constructible<CallbackResult, Result>::value &&
		!std::is_void<CallbackResultValue>::value,
	Result>
then(Awaitable &&awaitable, Callback &&callback) {
	auto awtr = wrap_awaiter(std::forward<Awaitable>(awaitable));
	if (awtr->await_ready()) {
		return std::forward<Callback>(callback)(awtr->await_resume());
	}
	struct ctx_t {
		promise<CallbackResultValue> prm;
		awaiter_wrapper<Awaitable &&> awtr;
		std::function<CallbackResult(AwaitableResult)> cb;
	};
	auto ctx = new ctx_t{{}, std::move(awtr), std::forward<Callback>(callback)};
	auto r = ctx->prm.get_future();
	if (await_suspend(*ctx->awtr, [ctx]() {
			then(
				ctx->cb(ctx->awtr->await_resume()),
				[ctx](CallbackResultValue val) {
					ctx->prm.deliver(std::move(val));
					delete ctx;
				});
		})) {
		return r;
	}
	r = ctx->cb(ctx->awtr->await_resume());
	delete ctx;
	return r;
}

template <
	typename Awaitable,
	typename Callback,
	typename AwaitableResult = await_result_t<Awaitable &&>,
	typename CallbackResult = invoke_result_t<Callback &&, AwaitableResult>,
	typename CallbackResultValue = await_result_t<CallbackResult>,
	typename Result = future<CallbackResultValue>>
inline enable_if_t<
	!std::is_void<AwaitableResult>::value &&
		!std::is_void<CallbackResult>::value &&
		std::is_constructible<CallbackResult, Result>::value &&
		std::is_void<CallbackResultValue>::value,
	Result>
then(Awaitable &&awaitable, Callback &&callback) {
	auto awtr = wrap_awaiter(std::forward<Awaitable>(awaitable));
	if (awtr->await_ready()) {
		return std::forward<Callback>(callback)(awtr->await_resume());
	}
	struct ctx_t {
		promise<> prm;
		awaiter_wrapper<Awaitable &&> awtr;
		std::function<CallbackResult(AwaitableResult)> cb;
	};
	auto ctx = new ctx_t{{}, std::move(awtr), std::forward<Callback>(callback)};
	auto r = ctx->prm.get_future();
	if (await_suspend(*ctx->awtr, [ctx]() {
			then(ctx->cb(ctx->awtr->await_resume()), [ctx]() {
				ctx->prm.deliver();
				delete ctx;
			});
		})) {
		return r;
	}
	r = ctx->cb(ctx->awtr->await_resume());
	delete ctx;
	return r;
}

template <
	typename Awaitable,
	typename Callback,
	typename AwaitableResult = await_result_t<Awaitable &&>,
	typename CallbackResult = invoke_result_t<Callback &&, AwaitableResult>,
	typename CallbackResultValue = await_result_t<CallbackResult>,
	typename Result = future<CallbackResultValue>>
inline enable_if_t<
	!std::is_void<AwaitableResult>::value &&
		!std::is_void<CallbackResult>::value &&
		!std::is_convertible<CallbackResult, Result>::value &&
		!std::is_void<CallbackResultValue>::value,
	Result>
then(Awaitable &&awaitable, Callback &&callback) {
	auto awtr = wrap_awaiter(std::forward<Awaitable>(awaitable));
	if (awtr->await_ready()) {
		return then(
			std::forward<Callback>(callback)(awtr->await_resume()),
			[](CallbackResultValue val) { return std::move(val); });
	}
	struct ctx_t {
		promise<CallbackResultValue> prm;
		awaiter_wrapper<Awaitable &&> awtr;
		std::function<CallbackResult(AwaitableResult)> cb;
	};
	auto ctx = new ctx_t{{}, std::move(awtr), std::forward<Callback>(callback)};
	auto r = ctx->prm.get_future();
	if (await_suspend(*ctx->awtr, [ctx]() {
			then(
				ctx->cb(ctx->awtr->await_resume()),
				[ctx](CallbackResultValue val) {
					ctx->prm.deliver(std::move(val));
					delete ctx;
				});
		})) {
		return r;
	}
	then(ctx->cb(ctx->awtr->await_resume()), [ctx](CallbackResultValue val) {
		ctx->prm.deliver(std::move(val));
		delete ctx;
	});
	return r;
}

template <
	typename Awaitable,
	typename Callback,
	typename AwaitableResult = await_result_t<Awaitable &&>,
	typename CallbackResult = invoke_result_t<Callback &&, AwaitableResult>,
	typename CallbackResultValue = await_result_t<CallbackResult>,
	typename Result = future<CallbackResultValue>>
inline enable_if_t<
	!std::is_void<AwaitableResult>::value &&
		!std::is_void<CallbackResult>::value &&
		!std::is_convertible<CallbackResult, Result>::value &&
		std::is_void<CallbackResultValue>::value,
	Result>
then(Awaitable &&awaitable, Callback &&callback) {
	auto awtr = wrap_awaiter(std::forward<Awaitable>(awaitable));
	if (awtr->await_ready()) {
		return then(
			std::forward<Callback>(callback)(awtr->await_resume()), []() {});
	}
	struct ctx_t {
		promise<> prm;
		awaiter_wrapper<Awaitable &&> awtr;
		std::function<CallbackResult(AwaitableResult)> cb;
	};
	auto ctx = new ctx_t{{}, std::move(awtr), std::forward<Callback>(callback)};
	auto r = ctx->prm.get_future();
	if (await_suspend(*ctx->awtr, [ctx]() {
			then(ctx->cb(ctx->awtr->await_resume()), [ctx]() {
				ctx->prm.deliver();
				delete ctx;
			});
		})) {
		return r;
	}
	then(ctx->cb(ctx->awtr->await_resume()), [ctx]() {
		ctx->prm.deliver();
		delete ctx;
	});
	return r;
}

template <
	typename Awaitable,
	typename Callback,
	typename AwaitableResult = await_result_t<Awaitable &&>,
	typename CallbackResult = invoke_result_t<Callback &&, AwaitableResult>>
inline enable_if_t<
	!std::is_void<AwaitableResult>::value &&
		std::is_void<CallbackResult>::value,
	future<CallbackResult>>
then(Awaitable &&awaitable, Callback &&callback) {
	auto awtr = wrap_awaiter(std::forward<Awaitable>(awaitable));
	if (awtr->await_ready()) {
		std::forward<Callback>(callback)(awtr->await_resume());
		return {};
	}
	struct ctx_t {
		promise<> prm;
		awaiter_wrapper<Awaitable &&> awtr;
		std::function<void(AwaitableResult)> cb;
	};
	auto ctx = new ctx_t{{}, std::move(awtr), std::forward<Callback>(callback)};
	auto r = ctx->prm.get_future();
	if (await_suspend(*ctx->awtr, [ctx]() {
			ctx->cb(ctx->awtr->await_resume());
			ctx->prm.deliver();
			delete ctx;
		})) {
		return r;
	}
	ctx->cb(ctx->awtr->await_resume());
	delete ctx;
	return {};
}

template <
	typename Awaitable,
	typename Callback,
	typename AwaitableResult = await_result_t<Awaitable &&>,
	typename CallbackResult = invoke_result_t<Callback &&>>
inline enable_if_t<
	std::is_void<AwaitableResult>::value &&
		!std::is_void<CallbackResult>::value &&
		!is_awaitable<CallbackResult>::value,
	future<CallbackResult>>
then(Awaitable &&awaitable, Callback &&callback) {
	auto awtr = wrap_awaiter(std::forward<Awaitable>(awaitable));
	if (awtr->await_ready()) {
		awtr->await_resume();
		return std::forward<Callback>(callback)();
	}
	struct ctx_t {
		promise<CallbackResult> prm;
		awaiter_wrapper<Awaitable &&> awtr;
		std::function<CallbackResult()> cb;
	};
	auto ctx = new ctx_t{{}, std::move(awtr), std::forward<Callback>(callback)};
	auto r = ctx->prm.get_future();
	if (await_suspend(*ctx->awtr, [ctx]() {
			ctx->awtr->await_resume();
			ctx->prm.deliver(ctx->cb());
			delete ctx;
		})) {
		return r;
	}
	ctx->awtr->await_resume();
	r = ctx->cb();
	delete ctx;
	return r;
}

template <
	typename Awaitable,
	typename Callback,
	typename AwaitableResult = await_result_t<Awaitable &&>,
	typename CallbackResult = invoke_result_t<Callback &&>,
	typename CallbackResultValue = await_result_t<CallbackResult>,
	typename Result = future<CallbackResultValue>>
inline enable_if_t<
	std::is_void<AwaitableResult>::value &&
		!std::is_void<CallbackResult>::value &&
		std::is_constructible<CallbackResult, Result>::value &&
		!std::is_void<CallbackResultValue>::value,
	Result>
then(Awaitable &&awaitable, Callback &&callback) {
	auto awtr = wrap_awaiter(std::forward<Awaitable>(awaitable));
	if (awtr->await_ready()) {
		awtr->await_resume();
		return std::forward<Callback>(callback)();
	}
	struct ctx_t {
		promise<CallbackResultValue> prm;
		awaiter_wrapper<Awaitable &&> awtr;
		std::function<CallbackResult()> cb;
	};
	auto ctx = new ctx_t{{}, std::move(awtr), std::forward<Callback>(callback)};
	auto r = ctx->prm.get_future();
	if (await_suspend(*ctx->awtr, [ctx]() {
			ctx->awtr->await_resume();
			then(ctx->cb(), [ctx](CallbackResultValue val) {
				ctx->prm.deliver(std::move(val));
				delete ctx;
			});
		})) {
		return r;
	}
	ctx->awtr->await_resume();
	r = ctx->cb();
	delete ctx;
	return r;
}

template <
	typename Awaitable,
	typename Callback,
	typename AwaitableResult = await_result_t<Awaitable &&>,
	typename CallbackResult = invoke_result_t<Callback &&>,
	typename CallbackResultValue = await_result_t<CallbackResult>,
	typename Result = future<CallbackResultValue>>
inline enable_if_t<
	std::is_void<AwaitableResult>::value &&
		!std::is_void<CallbackResult>::value &&
		std::is_constructible<CallbackResult, Result>::value &&
		std::is_void<CallbackResultValue>::value,
	Result>
then(Awaitable &&awaitable, Callback &&callback) {
	auto awtr = wrap_awaiter(std::forward<Awaitable>(awaitable));
	if (awtr->await_ready()) {
		awtr->await_resume();
		return std::forward<Callback>(callback)();
	}
	struct ctx_t {
		promise<> prm;
		awaiter_wrapper<Awaitable &&> awtr;
		std::function<CallbackResult()> cb;
	};
	auto ctx = new ctx_t{{}, std::move(awtr), std::forward<Callback>(callback)};
	auto r = ctx->prm.get_future();
	if (await_suspend(*ctx->awtr, [ctx]() {
			ctx->awtr->await_resume();
			then(ctx->cb(), [ctx]() {
				ctx->prm.deliver();
				delete ctx;
			});
		})) {
		return r;
	}
	ctx->awtr->await_resume();
	r = ctx->cb();
	delete ctx;
	return r;
}

template <
	typename Awaitable,
	typename Callback,
	typename AwaitableResult = await_result_t<Awaitable &&>,
	typename CallbackResult = invoke_result_t<Callback &&>,
	typename CallbackResultValue = await_result_t<CallbackResult>,
	typename Result = future<CallbackResultValue>>
inline enable_if_t<
	std::is_void<AwaitableResult>::value &&
		!std::is_void<CallbackResult>::value &&
		!std::is_constructible<CallbackResult, Result>::value &&
		!std::is_void<CallbackResultValue>::value,
	Result>
then(Awaitable &&awaitable, Callback &&callback) {
	auto awtr = wrap_awaiter(std::forward<Awaitable>(awaitable));
	if (awtr->await_ready()) {
		awtr->await_resume();
		return then(
			std::forward<Callback>(callback)(),
			[](CallbackResultValue val) { return std::move(val); });
	}
	struct ctx_t {
		promise<CallbackResultValue> prm;
		awaiter_wrapper<Awaitable &&> awtr;
		std::function<CallbackResult()> cb;
	};
	auto ctx = new ctx_t{{}, std::move(awtr), std::forward<Callback>(callback)};
	auto r = ctx->prm.get_future();
	if (await_suspend(*ctx->awtr, [ctx]() {
			ctx->awtr->await_resume();
			then(ctx->cb(), [ctx](CallbackResultValue val) {
				ctx->prm.deliver(std::move(val));
				delete ctx;
			});
		})) {
		return r;
	}
	ctx->awtr->await_resume();
	then(ctx->cb(), [ctx](CallbackResultValue val) {
		ctx->prm.deliver(std::move(val));
		delete ctx;
	});
	return r;
}

template <
	typename Awaitable,
	typename Callback,
	typename AwaitableResult = await_result_t<Awaitable &&>,
	typename CallbackResult = invoke_result_t<Callback &&>,
	typename CallbackResultValue = await_result_t<CallbackResult>,
	typename Result = future<CallbackResultValue>>
inline enable_if_t<
	std::is_void<AwaitableResult>::value &&
		!std::is_void<CallbackResult>::value &&
		!std::is_constructible<CallbackResult, Result>::value &&
		std::is_void<CallbackResultValue>::value,
	Result>
then(Awaitable &&awaitable, Callback &&callback) {
	auto awtr = wrap_awaiter(std::forward<Awaitable>(awaitable));
	if (awtr->await_ready()) {
		awtr->await_resume();
		return then(std::forward<Callback>(callback)(), []() {});
	}
	struct ctx_t {
		promise<> prm;
		awaiter_wrapper<Awaitable &&> awtr;
		std::function<CallbackResult()> cb;
	};
	auto ctx = new ctx_t{{}, std::move(awtr), std::forward<Callback>(callback)};
	auto r = ctx->prm.get_future();
	if (await_suspend(*ctx->awtr, [ctx]() {
			ctx->awtr->await_resume();
			then(ctx->cb(), [ctx]() {
				ctx->prm.deliver();
				delete ctx;
			});
		})) {
		return r;
	}
	ctx->awtr->await_resume();
	then(ctx->cb(), [ctx]() {
		ctx->prm.deliver();
		delete ctx;
	});
	return r;
}

template <
	typename Awaitable,
	typename Callback,
	typename AwaitableResult = await_result_t<Awaitable &&>,
	typename CallbackResult = invoke_result_t<Callback &&>>
inline enable_if_t<
	std::is_void<AwaitableResult>::value && std::is_void<CallbackResult>::value,
	future<CallbackResult>>
then(Awaitable &&awaitable, Callback &&callback) {
	auto awtr = wrap_awaiter(std::forward<Awaitable>(awaitable));
	if (awtr->await_ready()) {
		awtr->await_resume();
		std::forward<Callback>(callback)();
		return {};
	}
	struct ctx_t {
		promise<> prm;
		awaiter_wrapper<Awaitable &&> awtr;
		std::function<void()> cb;
	};
	auto ctx = new ctx_t{{}, std::move(awtr), std::forward<Callback>(callback)};
	auto r = ctx->prm.get_future();
	if (await_suspend(*ctx->awtr, [ctx]() {
			ctx->awtr->await_resume();
			ctx->cb();
			ctx->prm.deliver();
			delete ctx;
		})) {
		return r;
	}
	ctx->awtr->await_resume();
	ctx->cb();
	delete ctx;
	return {};
}

namespace await_operators {

template <typename Awaitable, typename Callback>
inline decltype(then(std::declval<Awaitable &&>(), std::declval<Callback &&>()))
operator|(Awaitable &&awaitable, Callback &&callback) {
	return then(
		std::forward<Awaitable>(awaitable), std::forward<Callback>(callback));
}

} // namespace await_operators

} // namespace rua

#endif
