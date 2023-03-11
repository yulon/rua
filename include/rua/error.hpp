#ifndef _RUA_ERROR_HPP
#define _RUA_ERROR_HPP

#include "interface_ptr.hpp"
#include "invocable.hpp"
#include "string/char_set.hpp"
#include "string/conv.hpp"
#include "string/join.hpp"
#include "util.hpp"
#include "variant.hpp"

#ifdef RUA_EXCEPTION_SUPPORTED

#include "thread/var.hpp"

#include <exception>

#endif

#include <cassert>
#include <functional>
#include <list>

namespace rua {

class error_base {
public:
	virtual ~error_base() = default;

	virtual ssize_t code() const {
		return static_cast<ssize_t>(std::hash<std::string>{}(info()));
	}

	virtual std::string info() const = 0;

protected:
	constexpr error_base() = default;
};

////////////////////////////////////////////////////////////////////////////

#ifdef RUA_EXCEPTION_SUPPORTED

class error_i : public interface_ptr<error_base>, public std::exception {
public:
	error_i() = default;

	RUA_CONSTRUCTIBLE_CONCEPT(Args, interface_ptr<error_base>, error_i)
	error_i(Args &&...args) :
		interface_ptr<error_base>(std::forward<Args>(args)...) {}

	error_i(const error_i &src) :
		interface_ptr<error_base>(
			static_cast<const interface_ptr<error_base> &>(src)) {}

	error_i(error_i &&src) :
		interface_ptr<error_base>(
			static_cast<interface_ptr<error_base> &&>(std::move(src))) {}

	RUA_OVERLOAD_ASSIGNMENT(error_i)

	virtual ~error_i() = default;

	const char *what() const RUA_NOEXCEPT override {
		// I really don't like std::exception
		static thread_var<std::string> what_cache;
		return what_cache.emplace(get()->info()).c_str();
	}
};

class exception_error : public error_base {
public:
	exception_error() = default;

	exception_error(std::exception_ptr ptr, std::string what) :
		_ptr(std::move(ptr)), _what(std::move(what)) {}

	virtual ~exception_error() = default;

	std::string info() const override {
		return _what;
	}

	void rethrow() {
		if (!_ptr) {
			return;
		}
		std::rethrow_exception(std::move(_ptr));
		_ptr = nullptr;
		_what.clear();
	}

private:
	std::exception_ptr _ptr;
	std::string _what;
};

template <typename Exception>
inline exception_error make_exception_error(Exception &&e) {
	std::string what(std::forward<Exception>(e).what());
	return exception_error(
		std::make_exception_ptr(std::forward<Exception>(e)), std::move(what));
}

inline exception_error current_exception_error(std::string what) {
	return exception_error(std::current_exception(), std::move(what));
}

inline exception_error current_exception_error(const std::exception &e) {
	std::string what(e.what());
	return exception_error(std::current_exception(), std::move(what));
}

#else

using error_i = interface_ptr<error_base>;

#endif

////////////////////////////////////////////////////////////////////////////

class str_error : public error_base {
public:
	str_error() = default;

	str_error(std::string s) : _s(std::move(s)) {}

	virtual ~str_error() = default;

	std::string info() const override {
		return _s;
	}

private:
	std::string _s;
};

class strv_error : public error_base {
public:
	constexpr strv_error() = default;

	constexpr strv_error(string_view sv) : _sv(sv) {}

	virtual ~strv_error() = default;

	std::string info() const override {
		return std::string(_sv);
	}

private:
	string_view _sv;
};

inline std::string to_string(const error_base &err) {
	return err.info();
}

inline std::string to_string(error_i err) {
	return err ? err->info() : "noerr";
}

////////////////////////////////////////////////////////////////////////////

RUA_CVAR strv_error unexpected("unexpected");

template <typename T = void>
class expected : public enable_value_operators<expected<T>, T> {
public:
	constexpr expected() = default;

	RUA_CONSTRUCTIBLE_CONCEPT(Args, RUA_ARG(variant<T, error_i>), expected)
	constexpr expected(Args &&...args) : _val(std::forward<Args>(args)...) {}

	expected(const expected &src) : _val(src._val) {}

	expected(expected &&src) : _val(std::move(src._val)) {}

	RUA_OVERLOAD_ASSIGNMENT(expected)

	bool has_value() const {
		return _val.template type_is<T>();
	}

	explicit operator bool() const {
		return has_value();
	}

	T &value() & {
#ifdef RUA_EXCEPTION_SUPPORTED
		if (!has_value()) {
			throw error();
		}
#endif
		assert(has_value());

		return _val.template as<T>();
	}

	T &&value() && {
#ifdef RUA_EXCEPTION_SUPPORTED
		if (!has_value()) {
			throw error();
		}
#endif
		assert(has_value());

		return std::move(_val).template as<T>();
	}

	const T &value() const & {
#ifdef RUA_EXCEPTION_SUPPORTED
		if (!has_value()) {
			throw error();
		}
#endif
		assert(has_value());

		return _val.template as<T>();
	}

	error_i error() const {
		if (_val.template type_is<error_i>()) {
			const auto &err = _val.template as<error_i>();
			assert(err);
			return err;
		}
		if (has_value()) {
			return nullptr;
		}
		return unexpected;
	}

	void reset() {
		_val.reset();
	}

	template <
		typename... Args,
		typename = enable_if_t<std::is_constructible<T, Args &&...>::value>>
	void emplace(Args &&...args) {
		_val.emplace(std::forward<Args>(args)...);
	}

	template <
		typename U,
		typename... Args,
		typename = enable_if_t<
			std::is_constructible<T, std::initializer_list<U>, Args &&...>::
				value>>
	void emplace(std::initializer_list<U> il, Args &&...args) {
		_val.emplace(il, std::forward<Args>(args)...);
	}

private:
	variant<T, error_i> _val;
};

template <>
class expected<void> {
public:
	expected() = default;

	RUA_CONSTRUCTIBLE_CONCEPT(ErrorArgs, error_i, expected)
	expected(ErrorArgs &&...err) : _err(std::forward<ErrorArgs>(err)...) {}

	expected(const expected &src) : _err(src._err) {}

	expected(expected &&src) : _err(std::move(src._err)) {}

	RUA_OVERLOAD_ASSIGNMENT(expected)

	bool has_value() const {
		return !_err;
	}

	explicit operator bool() const {
		return has_value();
	}

	RUA_CONSTEXPR_14 void value() const {}

	error_i error() const {
		return _err;
	}

	void reset() {
		_err.reset();
	}

	void emplace() {
		_err.reset();
	}

	void emplace(error_i err) {
		_err = std::move(err);
	}

private:
	error_i _err;
};

////////////////////////////////////////////////////////////////////////////

template <typename T>
struct is_expected : std::false_type {};

template <typename T>
struct is_expected<expected<T>> : std::true_type {};

#if RUA_CPP >= RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T>
inline constexpr auto is_expected_v = is_expected<T>::value;
#endif

////////////////////////////////////////////////////////////////////////////

template <typename T, typename = void>
struct warp_expected {
	using type = expected<T>;
};

template <typename T>
struct warp_expected<expected<T>, void> {
	using type = expected<T>;
};

template <typename T>
struct warp_expected<T, enable_if_t<std::is_base_of<error_base, T>::value>> {
	using type = expected<>;
};

template <typename T>
struct warp_expected<T, enable_if_t<std::is_base_of<error_i, T>::value>> {
	using type = expected<>;
};

template <typename T>
using warp_expected_t = typename warp_expected<T>::type;

template <typename T>
struct unwarp_expected {
	using type = T;
};

template <typename T>
struct unwarp_expected<expected<T>> {
	using type = T;
};

template <typename T>
using unwarp_expected_t = typename unwarp_expected<T>::type;

////////////////////////////////////////////////////////////////////////////

// INVOKE(F, expected<T> => expected<T>) -> non-void => expected<non-void>
template <
	typename F,
	typename T,
	typename RawR = decay_t<invoke_result_t<F &&, expected<T>>>,
	typename ExpR = warp_expected_t<RawR>>
inline enable_if_t<!std::is_void<RawR>::value, ExpR>
expected_invoke(F &&f, expected<T> exp) {
#ifdef RUA_EXCEPTION_SUPPORTED
	try {
#endif
		return rua::invoke(std::forward<F>(f), std::move(exp));
#ifdef RUA_EXCEPTION_SUPPORTED
	} catch (const std::exception &e) {
		return current_exception_error(e);
	}
#endif
}

// INVOKE(F, expected<T> => expected<T>) -> void => expected<void>
template <
	typename F,
	typename T,
	typename RawR = decay_t<invoke_result_t<F &&, expected<T>>>,
	typename ExpR = warp_expected_t<RawR>>
inline enable_if_t<std::is_void<RawR>::value, ExpR>
expected_invoke(F &&f, expected<T> exp) {
#ifdef RUA_EXCEPTION_SUPPORTED
	try {
#endif
		rua::invoke(std::forward<F>(f), std::move(exp));
#ifdef RUA_EXCEPTION_SUPPORTED
	} catch (const std::exception &e) {
		return current_exception_error(e);
	}
#endif
	return nullptr;
}

// INVOKE(F, expected<non-void> => expected<void>) -> non-void =>
// expected<non-void>
template <
	typename F,
	typename T,
	typename RawR = decay_t<invoke_result_t<F &&, expected<void>>>,
	typename ExpR = warp_expected_t<RawR>>
inline enable_if_t<
	!std::is_void<RawR>::value && !std::is_void<T>::value &&
		!is_invocable<F &&, expected<T>>::value,
	ExpR>
expected_invoke(F &&f, expected<T> exp) {
#ifdef RUA_EXCEPTION_SUPPORTED
	try {
#endif
		return rua::invoke(std::forward<F>(f), expected<void>(exp.error()));
#ifdef RUA_EXCEPTION_SUPPORTED
	} catch (const std::exception &e) {
		return current_exception_error(e);
	}
#endif
}

// INVOKE(F, expected<non-void> => expected<void>) -> void => expected<void>
template <
	typename F,
	typename T,
	typename RawR = decay_t<invoke_result_t<F &&, expected<void>>>,
	typename ExpR = warp_expected_t<RawR>>
inline enable_if_t<
	std::is_void<RawR>::value && !std::is_void<T>::value &&
		!is_invocable<F &&, expected<T>>::value,
	ExpR>
expected_invoke(F &&f, expected<T> exp) {
#ifdef RUA_EXCEPTION_SUPPORTED
	try {
#endif
		rua::invoke(std::forward<F>(f), expected<void>(exp.error()));
#ifdef RUA_EXCEPTION_SUPPORTED
	} catch (const std::exception &e) {
		return current_exception_error(e);
	}
#endif
	return nullptr;
}

// INVOKE(F, expected<T> => error_i) -> non-void => expected<non-void>
template <
	typename F,
	typename T,
	typename RawR = decay_t<invoke_result_t<F &&, error_i>>,
	typename ExpR = warp_expected_t<RawR>>
inline enable_if_t<
	!std::is_void<RawR>::value && !is_invocable<F &&, expected<T>>::value &&
		(std::is_void<T>::value || !is_invocable<F &&, expected<void>>::value),
	ExpR>
expected_invoke(F &&f, expected<T> exp) {
#ifdef RUA_EXCEPTION_SUPPORTED
	try {
#endif
		return rua::invoke(std::forward<F>(f), exp.error());
#ifdef RUA_EXCEPTION_SUPPORTED
	} catch (const std::exception &e) {
		return current_exception_error(e);
	}
#endif
}

// INVOKE(F, expected<T> => error_i) -> void => expected<void>
template <
	typename F,
	typename T,
	typename RawR = decay_t<invoke_result_t<F &&, error_i>>,
	typename ExpR = warp_expected_t<RawR>>
inline enable_if_t<
	std::is_void<RawR>::value && !is_invocable<F &&, expected<T>>::value &&
		(std::is_void<T>::value ||
		 !is_invocable<F &&, expected<void>>::value) &&
		!is_invocable<F &&, T>::value,
	ExpR>
expected_invoke(F &&f, expected<T> exp) {
#ifdef RUA_EXCEPTION_SUPPORTED
	try {
#endif
		rua::invoke(std::forward<F>(f), exp.error());
#ifdef RUA_EXCEPTION_SUPPORTED
	} catch (const std::exception &e) {
		return current_exception_error(e);
	}
#endif
	return nullptr;
}

// INVOKE(F, expected<T> => T) -> non-void => expected<non-void>
// F is not invoked when input error.
template <
	typename F,
	typename T,
	typename RawR = decay_t<invoke_result_t<F &&, T>>,
	typename ExpR = warp_expected_t<RawR>>
inline enable_if_t<
	!std::is_void<RawR>::value && !is_invocable<F &&, expected<T>>::value &&
		(std::is_void<T>::value ||
		 !is_invocable<F &&, expected<void>>::value) &&
		!is_invocable<F &&, error_i>::value,
	ExpR>
expected_invoke(F &&f, expected<T> exp) {
	if (!exp) {
		return exp.error();
	}
#ifdef RUA_EXCEPTION_SUPPORTED
	try {
#endif
		return rua::invoke(std::forward<F>(f), std::move(exp).value());
#ifdef RUA_EXCEPTION_SUPPORTED
	} catch (const std::exception &e) {
		return current_exception_error(e);
	}
#endif
}

// INVOKE(F, expected<T> => T) -> void => expected<void>
// F is not invoked when input error.
template <
	typename F,
	typename T,
	typename RawR = decay_t<invoke_result_t<F &&, T>>,
	typename ExpR = warp_expected_t<RawR>>
inline enable_if_t<
	std::is_void<RawR>::value && !is_invocable<F &&, expected<T>>::value &&
		(std::is_void<T>::value ||
		 !is_invocable<F &&, expected<void>>::value) &&
		!is_invocable<F &&, error_i>::value,
	ExpR>
expected_invoke(F &&f, expected<T> exp) {
	if (!exp) {
		return exp.error();
	}
#ifdef RUA_EXCEPTION_SUPPORTED
	try {
#endif
		rua::invoke(std::forward<F>(f), std::move(exp).value());
#ifdef RUA_EXCEPTION_SUPPORTED
	} catch (const std::exception &e) {
		return current_exception_error(e);
	}
#endif
	return nullptr;
}

// INVOKE(F, expected<T> => T || error_i) -> non-void => expected<non-void>
template <
	typename F,
	typename T,
	typename RawR = decay_t<invoke_result_t<F &&, T>>,
	typename ExpR = warp_expected_t<RawR>>
inline enable_if_t<
	!std::is_void<RawR>::value && !is_invocable<F &&, expected<T>>::value &&
		(std::is_void<T>::value ||
		 !is_invocable<F &&, expected<void>>::value) &&
		is_invocable<F &&, error_i>::value,
	ExpR>
expected_invoke(F &&f, expected<T> exp) {
#ifdef RUA_EXCEPTION_SUPPORTED
	try {
#endif
		if (exp.has_value()) {
			return rua::invoke(std::forward<F>(f), std::move(exp).value());
		}
		return rua::invoke(std::forward<F>(f), exp.error());
#ifdef RUA_EXCEPTION_SUPPORTED
	} catch (const std::exception &e) {
		return current_exception_error(e);
	}
#endif
}

// INVOKE(F, expected<T> => T || error_i) -> void => expected<void>
template <
	typename F,
	typename T,
	typename RawR = decay_t<invoke_result_t<F &&, T>>,
	typename ExpR = warp_expected_t<RawR>>
inline enable_if_t<
	std::is_void<RawR>::value && !is_invocable<F &&, expected<T>>::value &&
		(std::is_void<T>::value ||
		 !is_invocable<F &&, expected<void>>::value) &&
		is_invocable<F &&, error_i>::value,
	ExpR>
expected_invoke(F &&f, expected<T> exp) {
#ifdef RUA_EXCEPTION_SUPPORTED
	try {
#endif
		if (exp.has_value()) {
			rua::invoke(std::forward<F>(f), std::move(exp).value());
		} else {
			rua::invoke(std::forward<F>(f), exp.error());
		}
#ifdef RUA_EXCEPTION_SUPPORTED
	} catch (const std::exception &e) {
		return current_exception_error(e);
	}
#endif
	return nullptr;
}

// INVOKE(F, expected<T> => void) -> non-void => expected<non-void>
// F is not invoked when input error.
template <
	typename F,
	typename T,
	typename RawR = decay_t<invoke_result_t<F &&>>,
	typename ExpR = warp_expected_t<RawR>>
inline enable_if_t<
	!std::is_void<RawR>::value && !is_invocable<F &&, expected<T>>::value &&
		(std::is_void<T>::value ||
		 !is_invocable<F &&, expected<void>>::value) &&
		!is_invocable<F &&, error_i>::value && !is_invocable<F &&, T>::value,
	ExpR>
expected_invoke(F &&f, expected<T> exp) {
	if (!exp) {
		return exp.error();
	}
#ifdef RUA_EXCEPTION_SUPPORTED
	try {
#endif
		return rua::invoke(std::forward<F>(f));
#ifdef RUA_EXCEPTION_SUPPORTED
	} catch (const std::exception &e) {
		return current_exception_error(e);
	}
#endif
}

// INVOKE(F, expected<T> => void) -> void => expected<void>
// F is not invoked when input error.
template <
	typename F,
	typename T,
	typename RawR = decay_t<invoke_result_t<F &&>>,
	typename ExpR = warp_expected_t<RawR>>
inline enable_if_t<
	std::is_void<RawR>::value && !is_invocable<F &&, expected<T>>::value &&
		(std::is_void<T>::value ||
		 !is_invocable<F &&, expected<void>>::value) &&
		!is_invocable<F &&, error_i>::value && !is_invocable<F &&, T>::value,
	ExpR>
expected_invoke(F &&f, expected<T> exp) {
	if (!exp) {
		return exp.error();
	}
#ifdef RUA_EXCEPTION_SUPPORTED
	try {
#endif
		rua::invoke(std::forward<F>(f));
#ifdef RUA_EXCEPTION_SUPPORTED
	} catch (const std::exception &e) {
		return current_exception_error(e);
	}
#endif
	return nullptr;
}

// INVOKE(F, T => expected<T>) -> R => expected<R>
template <
	typename F,
	typename T,
	typename Val = decay_t<T>,
	typename Exp = enable_if_t<
		!is_expected<Val>::value && !is_invocable<T &&>::value,
		warp_expected_t<Val>>>
inline decltype(expected_invoke(std::declval<F &&>(), std::declval<Exp>()))
expected_invoke(F &&f, T &&arg) {
	return expected_invoke(std::forward<F>(f), Exp(std::forward<T>(arg)));
}

// INVOKE(F, GetVal() -> T => expected<T>) -> R => expected<R>
template <
	typename F,
	typename GetVal,
	typename... Args,
	typename Exp =
		warp_expected_t<decay_t<invoke_result_t<GetVal &&, Args &&...>>>>
inline decltype(expected_invoke(std::declval<F &&>(), std::declval<Exp>()))
expected_invoke(F &&f, GetVal &&get_val, Args &&...args) {
	Exp exp;
#ifdef RUA_EXCEPTION_SUPPORTED
	try {
#endif
		exp = rua::invoke(
			std::forward<GetVal>(get_val), std::forward<Args>(args)...);
#ifdef RUA_EXCEPTION_SUPPORTED
	} catch (const std::exception &e) {
		exp = current_exception_error(e);
	}
#endif
	return expected_invoke(std::forward<F>(f), std::move(exp));
}

// INVOKE(F, void => expected<void>) -> R => expected<R>
template <typename F>
inline decltype(expected_invoke(
	std::declval<F &&>(), std::declval<expected<void>>()))
expected_invoke(F &&f) {
	return expected_invoke(std::forward<F>(f), expected<void>());
}

} // namespace rua

namespace std {

template <>
class hash<rua::error_i> {
public:
	size_t operator()(rua::error_i err) const {
		return static_cast<size_t>(err->code());
	}
};

template <>
class less<rua::error_i> {
public:
	bool operator()(rua::error_i lhs, rua::error_i rhs) const {
		return lhs->code() < rhs->code();
	}
};

} // namespace std

#endif
