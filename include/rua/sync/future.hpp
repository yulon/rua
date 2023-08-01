#ifndef _rua_sync_future_hpp
#define _rua_sync_future_hpp

#include "await.hpp"
#include "promise.hpp"

#include "../coroutine.hpp"
#include "../dype/variant.hpp"
#include "../error.hpp"
#include "../util.hpp"

#include <atomic>
#include <cassert>
#include <functional>
#include <memory>

namespace rua {

RUA_CVAR strv_error err_unpromised("unpromised");

template <typename Promise, typename PromiseValue>
struct _future_promise_returner {
	void return_value(PromiseValue val) noexcept {
		static_cast<promise<PromiseValue> *>(static_cast<Promise *>(this))
			->fulfill(std::move(val));
	}
};

template <typename Promise>
struct _future_promise_returner<Promise, void> {
	void return_void() noexcept {
		static_cast<promise<> *>(static_cast<Promise *>(this))->fulfill();
	}
};

template <typename T = void, typename PromiseValue = T>
class future : private enable_await_operators {
public:
	class promise_type
		: private promise<PromiseValue>,
		  public _future_promise_returner<promise_type, PromiseValue> {
		friend _future_promise_returner<promise_type, PromiseValue>;

	public:
		virtual ~promise_type() = default;

		future get_return_object() noexcept {
			return *static_cast<promise<PromiseValue> *>(this);
		}

		static future get_return_object_on_allocation_failure() noexcept {
			return err_unpromised;
		}

		suspend_never initial_suspend() noexcept {
			return {};
		}

		suspend_always final_suspend() noexcept {
			return {};
		}

#ifdef RUA_HAS_EXCEPTIONS
		void unhandled_exception() noexcept {
			this->fulfill(current_exception_error());
		}
#endif

	protected:
		void on_destroy() noexcept override {
			coroutine_handle<promise_type>::from_promise(*this).destroy();
		}
	};

	////////////////////////////////////////////////////////////////////////

	constexpr future() : $v(err_unpromised) {}

	RUA_TMPL_FWD_CTOR(Args, RUA_A(expected<T>), future)
	future(Args &&...args) : $v(std::forward<Args>(args)...) {}

	RUA_TMPL_FWD_CTOR_IL(U, Args, T)
	future(std::initializer_list<U> il, Args &&...args) :
		$v(in_place_type_t<T>{}, il, std::forward<Args>(args)...) {}

	template <
		typename U = T,
		typename = enable_if_t<
			(std::is_void<T>::value && std::is_void<U>::value) ||
			std::is_convertible<U, T>::value>>
	future(expected<U> exp) : $v(std::move(exp).vrt()) {}

	template <
		typename Promise,
		typename = enable_if_t<
			std::is_convertible<Promise &, promise<PromiseValue> &>::value>>
	future(Promise &prm) : $v(&static_cast<promise<PromiseValue> &>(prm)) {}

	~future() {
		reset();
	}

	template <
		typename U,
		typename = enable_if_t<
			!std::is_same<T, U>::value && std::is_convertible<U &&, T>::value &&
			!std::is_convertible<future<U, PromiseValue> &&, T>::value>>
	future(future<U, PromiseValue> &&src) : $v(std::move(src.$v)) {}

	template <
		typename U,
		typename = enable_if_t<!std::is_convertible<U &&, T>::value>,
		typename = enable_if_t<
			!std::is_convertible<future<U, PromiseValue> &&, T>::value>>
	future(future<U, PromiseValue> &&src) :
		$v(*std::move(src.$v).template as<promise<PromiseValue> *>()) {}

	future(future &&src) : $v(std::move(src.$v)) {}

	RUA_OVERLOAD_ASSIGNMENT(future);

	bool await_ready() const noexcept {
		return !$v.template type_is<promise<PromiseValue> *>() ||
			   $v.template as<promise<PromiseValue> *>()->await_ready();
	}

	bool await_suspend(std::function<void()> notify) noexcept {
		assert($v.template type_is<promise<PromiseValue> *>());

		return $v.template as<promise<PromiseValue> *>()->await_suspend(
			std::move(notify));
	}

	expected<T> await_resume() noexcept {
		if (!$v) {
			return unexpected;
		}
		if ($v.template type_is<promise<PromiseValue> *>()) {
			auto prm = $v.template as<promise<PromiseValue> *>();
			$v.reset();
			return prm->await_resume();
		}
		return std::move($v);
	}

	void reset() noexcept {
		if ($v.template type_is<promise<PromiseValue> *>()) {
			$v.template as<promise<PromiseValue> *>()->unharvest();
		}
		$v.reset();
	}

private:
	variant<T, error_i, promise<PromiseValue> *> $v;
};

} // namespace rua

#endif
