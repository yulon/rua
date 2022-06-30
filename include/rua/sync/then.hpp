#ifndef _RUA_SYNC_THEN_HPP
#define _RUA_SYNC_THEN_HPP

#include "await.hpp"
#include "future.hpp"

#include "../skater.hpp"

#include <functional>

namespace rua {

template <
	typename Awaitable,
	typename Callback,
	typename AwaitableResult,
	typename CallbackResult>
struct _thener {
	static future<CallbackResult>
	then(Awaitable &&awaitable, Callback &&callback) {
		auto &&awaiter = make_awaiter(std::forward<Awaitable>(awaitable));
		if (awaiter.await_ready()) {
			return std::forward<Callback>(callback)(awaiter.await_resume());
		}
		struct ctx_t {
			promise<CallbackResult> pms;
			decay_t<decltype(awaiter)> awtr;
			std::function<CallbackResult(AwaitableResult)> cb;
		};
		auto ctx =
			new ctx_t{{}, std::move(awaiter), std::forward<Callback>(callback)};
		future<CallbackResult> r = ctx->pms.get_promising_future();
		if (await_suspend(ctx->awtr, [ctx]() mutable {
				ctx->pms.set_value(ctx->cb(ctx->awtr.await_resume()));
				delete ctx;
			})) {
			return r;
		}
		r = ctx->cb(ctx->awtr.await_resume());
		delete ctx;
		return r;
	}
};

template <typename Awaitable, typename Callback, typename AwaitableResult>
struct _thener<Awaitable, Callback, AwaitableResult, void> {
	static future<> then(Awaitable &&awaitable, Callback &&callback) {
		auto &&awaiter = make_awaiter(std::forward<Awaitable>(awaitable));
		if (awaiter.await_ready()) {
			std::forward<Callback>(callback)(awaiter.await_resume());
			return {};
		}
		struct ctx_t {
			promise<> pms;
			decay_t<decltype(awaiter)> awtr;
			std::function<void(AwaitableResult)> cb;
		};
		auto ctx =
			new ctx_t{{}, std::move(awaiter), std::forward<Callback>(callback)};
		future<> r = ctx->pms.get_promising_future();
		if (await_suspend(ctx->awtr, [ctx]() mutable {
				ctx->cb(ctx->awtr.await_resume());
				ctx->pms.set_value();
				delete ctx;
			})) {
			return r;
		}
		ctx->cb(ctx->awtr.await_resume());
		delete ctx;
		return {};
	}
};

template <typename Awaitable, typename Callback, typename CallbackResult>
struct _thener<Awaitable, Callback, void, CallbackResult> {
	static future<CallbackResult>
	then(Awaitable &&awaitable, Callback &&callback) {
		auto &&awaiter = make_awaiter(std::forward<Awaitable>(awaitable));
		if (awaiter.await_ready()) {
			awaiter.await_resume();
			return std::forward<Callback>(callback)();
		}
		struct ctx_t {
			promise<CallbackResult> pms;
			decay_t<decltype(awaiter)> awtr;
			std::function<CallbackResult()> cb;
		};
		auto ctx =
			new ctx_t{{}, std::move(awaiter), std::forward<Callback>(callback)};
		future<CallbackResult> r = ctx->pms.get_promising_future();
		if (await_suspend(ctx->awtr, [ctx]() mutable {
				ctx->awtr.await_resume();
				ctx->pms.set_value(ctx->cb());
				delete ctx;
			})) {
			return r;
		}
		ctx->awtr.await_resume();
		r = ctx->cb();
		delete ctx;
		return r;
	}
};

template <typename Awaitable, typename Callback>
struct _thener<Awaitable, Callback, void, void> {
	static future<> then(Awaitable &&awaitable, Callback &&callback) {
		auto &&awaiter = make_awaiter(std::forward<Awaitable>(awaitable));
		if (awaiter.await_ready()) {
			awaiter.await_resume();
			std::forward<Callback>(callback)();
			return {};
		}
		struct ctx_t {
			promise<> pms;
			decay_t<decltype(awaiter)> awtr;
			std::function<void()> cb;
		};
		auto ctx =
			new ctx_t{{}, std::move(awaiter), std::forward<Callback>(callback)};
		future<> r = ctx->pms.get_promising_future();
		if (await_suspend(ctx->awtr, [ctx]() mutable {
				ctx->awtr.await_resume();
				ctx->cb();
				ctx->pms.set_value();
				delete ctx;
			})) {
			return r;
		}
		ctx->awtr.await_resume();
		ctx->cb();
		delete ctx;
		return {};
	}
};

template <
	typename Awaitable,
	typename Callback,
	typename AwaitableResult = await_result_t<Awaitable &&>,
	typename CallbackResult = invoke_result_t<Callback &&, AwaitableResult>>
inline future<CallbackResult> then(Awaitable &&awaitable, Callback &&callback) {
	return _thener<Awaitable, Callback, AwaitableResult, CallbackResult>::then(
		std::forward<Awaitable>(awaitable), std::forward<Callback>(callback));
}

} // namespace rua

#endif
