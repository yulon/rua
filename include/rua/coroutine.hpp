#ifndef _rua_coroutine_hpp
#define _rua_coroutine_hpp

#include "util.hpp"

#include <cassert>

#ifdef __cpp_lib_coroutine

#include <coroutine>

namespace rua {

template <typename Promise = void>
using coroutine_handle = std::coroutine_handle<Promise>;

using suspend_never = std::suspend_never;
using suspend_always = std::suspend_always;

} // namespace rua

#else

#include <cstdlib>

namespace rua {

template <typename FakeCoroutineHandle>
class _fake_coroutine_handle_base {
public:
	static FakeCoroutineHandle from_address(const void *) noexcept {
		abort();
		return nullptr;
	}

	////////////////////////////////////////////////////////////////////////

	void *address() const noexcept {
		abort();
		return nullptr;
	}

	explicit operator bool() const noexcept {
		abort();
		return false;
	}

	bool done() const noexcept {
		abort();
		return false;
	}

	void resume() const {
		abort();
	}

	void operator()() const {
		resume();
	}

	void destroy() {
		abort();
	}
};

template <typename Promise = void>
class _fake_coroutine_handle
	: public _fake_coroutine_handle_base<_fake_coroutine_handle<Promise> > {
public:
	static _fake_coroutine_handle from_promise(Promise &) noexcept {
		abort();
		return nullptr;
	}

	////////////////////////////////////////////////////////////////////////

	constexpr _fake_coroutine_handle(std::nullptr_t = nullptr) noexcept {}

	Promise &promise() noexcept {
		abort();
		return *reinterpret_cast<Promise *>(this);
	}
};

template <>
class _fake_coroutine_handle<void>
	: public _fake_coroutine_handle_base<_fake_coroutine_handle<void> > {
public:
	constexpr _fake_coroutine_handle(std::nullptr_t = nullptr) noexcept {}
};

template <typename Promise = void>
using coroutine_handle = _fake_coroutine_handle<Promise>;

struct suspend_never {
	constexpr bool await_ready() const noexcept {
		return true;
	}
	void await_suspend(coroutine_handle<>) const noexcept {}
	void await_resume() const noexcept {}
};

struct suspend_always {
	constexpr bool await_ready() const noexcept {
		return false;
	}
	void await_suspend(coroutine_handle<>) const noexcept {}
	void await_resume() const noexcept {}
};

} // namespace rua

#endif

namespace rua {

struct suspend_and_destroy {
	constexpr bool await_ready() const noexcept {
		return false;
	}

	void await_suspend(coroutine_handle<> co) const noexcept {
		co.destroy();
	}

	void await_resume() const noexcept {}
};

} // namespace rua

#endif
