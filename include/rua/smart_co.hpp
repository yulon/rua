#ifndef _RUA_SMART_CO_HPP
#define _RUA_SMART_CO_HPP

#ifdef __cpp_lib_coroutine

#include <cassert>
#include <coroutine>
#include <memory>

namespace rua {

template <typename Promise>
class observer_co : public std::coroutine_handle<Promise> {
public:
	constexpr observer_co() noexcept = default;

	constexpr observer_co(std::nullptr_t) noexcept :
		std::coroutine_handle<Promise>() {}

	constexpr observer_co(std::coroutine_handle<Promise> h) noexcept :
		std::coroutine_handle<Promise>(h) {}

	constexpr explicit observer_co(Promise &p) noexcept :
		std::coroutine_handle<Promise>(
			std::coroutine_handle<Promise>::from_promise(p)) {}

	constexpr explicit observer_co(void *ptr) noexcept :
		std::coroutine_handle<Promise>(
			std::coroutine_handle<Promise>::from_address(ptr)) {}

	std::coroutine_handle<Promise> get() const noexcept {
		return *this;
	}

	Promise &operator*() const noexcept {
		return this->promise();
	}

	Promise *operator->() const noexcept {
		return &this->promise();
	}
};

struct co_destroy {
	void operator()(void *ptr) const {
		std::coroutine_handle<>::from_address(ptr).destroy();
	}
};

template <typename Promise>
class unique_co {
public:
	constexpr unique_co() noexcept = default;

	constexpr unique_co(std::nullptr_t) noexcept : _ptr() {}

	explicit unique_co(Promise &p) noexcept :
		_ptr(std::coroutine_handle<Promise>::from_promise(p).address()) {}

	explicit unique_co(void *ptr) noexcept : _ptr(ptr) {}

	void *address() const noexcept {
		return _ptr.get();
	}

	std::coroutine_handle<Promise> get() const noexcept {
		return std::coroutine_handle<Promise>::from_address(address());
	}

	operator std::coroutine_handle<>() const noexcept {
		return std::coroutine_handle<>::from_address(address());
	}

	operator bool() const noexcept {
		return _ptr.get();
	}

	bool done() const noexcept {
		return get().done();
	}

	void operator()() const {
		get()();
	}

	void resume() const {
		get().resume();
	}

	void reset() noexcept {
		_ptr.reset();
	}

	Promise &promise() const noexcept {
		return get().promise();
	}

	Promise *operator->() const noexcept {
		return &get().promise();
	}

private:
	std::unique_ptr<void, co_destroy> _ptr;
};

template <typename Promise>
class shared_co {
public:
	constexpr shared_co() noexcept = default;

	constexpr shared_co(std::nullptr_t) noexcept : _ptr() {}

	explicit shared_co(Promise &p) :
		_ptr(
			std::coroutine_handle<Promise>::from_promise(p).address(),
			co_destroy{}) {}

	explicit shared_co(void *ptr) : _ptr(ptr, co_destroy{}) {}

	void *address() const noexcept {
		return _ptr.get();
	}

	std::coroutine_handle<Promise> get() const noexcept {
		return std::coroutine_handle<Promise>::from_address(address());
	}

	operator std::coroutine_handle<>() const noexcept {
		return std::coroutine_handle<>::from_address(address());
	}

	operator bool() const noexcept {
		return _ptr.get();
	}

	bool done() const noexcept {
		return get().done();
	}

	void operator()() const {
		get()();
	}

	void resume() const {
		get().resume();
	}

	void reset() noexcept {
		_ptr.reset();
	}

	Promise &promise() const noexcept {
		return get().promise();
	}

	Promise *operator->() const noexcept {
		return &get().promise();
	}

private:
	std::shared_ptr<void> _ptr;
};

} // namespace rua

#endif

#endif