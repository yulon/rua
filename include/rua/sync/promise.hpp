#ifndef _rua_sync_promise_hpp
#define _rua_sync_promise_hpp

#include "await.hpp"

#include "../error.hpp"
#include "../util.hpp"

#include <atomic>
#include <cassert>
#include <functional>
#include <memory>

namespace rua {

enum class promise_state : uintptr_t {
	loss_notify,
	has_notify,
	fulfilled,
	harvested
#ifndef NDEBUG
	,
	destroying
#endif
};

RUA_CVAR strv_error err_promise_unfulfilled("promise unfulfilled");
RUA_CVAR strv_error err_promise_not_yet_fulfilled("promise not yet fulfilled");
RUA_CVAR strv_error err_promise_fulfilled("promise fulfilled");

template <typename T = void, typename Extend = void>
class promise;

template <typename T>
class promise<T, void> {
public:
	promise() : $state(promise_state::loss_notify) {}

	promise(const promise &) = delete;
	promise(promise &&) = delete;
	promise &operator=(const promise &) = delete;
	promise &operator=(promise &&) = delete;

	virtual ~promise() {
#ifndef NDEBUG
		auto old_state = $state.load();
		assert(old_state == promise_state::destroying);
#endif
	}

	//////////////////// fulfill ////////////////////

	expected<T> fulfill(
		expected<T> value = expected_or<T>(err_promise_unfulfilled)) noexcept {

		assert(std::is_void<T>::value || !$val);

		$val = std::move(value);

		auto old_state = $state.exchange(promise_state::fulfilled);

		assert(old_state != promise_state::fulfilled);

		expected<T> refunded;

		switch (old_state) {

		case promise_state::harvested:
			assert(
				$state.exchange(promise_state::destroying) ==
				promise_state::fulfilled);
			refunded = std::move($val);
			on_destroy();
			break;

		case promise_state::has_notify: {
			assert($notify);
			auto notify = std::move($notify);
			notify();
			RUA_FALLTHROUGH;
		}

		default:
			refunded = err_promise_fulfilled;
			break;
		}

		return refunded;
	}

	void unfulfill() noexcept {
		fulfill(err_promise_unfulfilled);
	}

	////////////////// harvesting //////////////////

	bool await_ready() const noexcept {
		assert($state.load() != promise_state::harvested);

		return $state.load() == promise_state::fulfilled;
	}

	bool await_suspend(std::function<void()> notify) noexcept {
		$notify = std::move(notify);

		auto old_state = $state.load();
		if (old_state == promise_state::loss_notify) {
			$state.compare_exchange_strong(
				old_state, promise_state::has_notify);
		}

		assert(old_state != promise_state::has_notify);
		assert(old_state != promise_state::harvested);
		assert(
			old_state == promise_state::loss_notify ||
			old_state == promise_state::fulfilled);

		return old_state != promise_state::fulfilled;
	}

	expected<T> await_resume() noexcept {
		expected<T> r;

		auto old_state = $state.exchange(promise_state::harvested);

		assert(old_state != promise_state::harvested);

		if (old_state == promise_state::fulfilled) {
			assert(
				$state.exchange(promise_state::destroying) ==
				promise_state::harvested);
			r = std::move($val);
			on_destroy();
		} else {
			r = err_promise_not_yet_fulfilled;
		}
		return r;
	}

	//////////////////// unused ////////////////////

	void unfulfill_and_harvest() noexcept {
		unfulfill();
		await_resume();
	}

protected:
	virtual void on_destroy() noexcept {}

private:
	std::atomic<promise_state> $state;
	expected<T> $val;
	std::function<void()> $notify;
};

template <typename T, typename Extend>
class promise : public promise<T, void> {
public:
	promise() = default;

	RUA_TMPL_FWD_CTOR(Args, Extend, promise)
	explicit promise(Args &&...args) :
		promise<T, void>(), $ext(std::forward<Args>(args)...) {}

	RUA_TMPL_FWD_CTOR_IL(U, Args, Extend)
	explicit promise(std::initializer_list<U> il, Args &&...args) :
		promise<T, void>(), $ext(il, std::forward<Args>(args)...) {}

	virtual ~promise() = default;

	Extend &extend() & {
		return $ext;
	}

	Extend &&extend() && {
		return std::move($ext);
	}

	const Extend &extend() const & {
		return $ext;
	}

private:
	Extend $ext;
};

template <typename T = void, typename Extend = void>
class newable_promise : public promise<T, Extend> {
public:
	newable_promise() = default;

	RUA_TMPL_FWD_CTOR(Args, RUA_A(promise<T, Extend>), newable_promise)
	explicit newable_promise(Args &&...args) :
		promise<T, Extend>(std::forward<Args>(args)...) {}

	RUA_TMPL_FWD_CTOR_IL(U, Args, RUA_A(promise<T, Extend>))
	explicit newable_promise(std::initializer_list<U> il, Args &&...args) :
		promise<T, Extend>(il, std::forward<Args>(args)...) {}

	virtual ~newable_promise() = default;

	void on_destroy() noexcept override {
		delete this;
	}
};

} // namespace rua

#endif
