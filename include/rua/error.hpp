#ifndef _rua_error_hpp
#define _rua_error_hpp

#include "dype/interface_ptr.hpp"
#include "dype/variant.hpp"
#include "invocable.hpp"
#include "string/char_set.hpp"
#include "string/conv.hpp"
#include "string/join.hpp"
#include "util.hpp"

#ifdef RUA_HAS_EXCEPTIONS

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

#ifdef RUA_HAS_EXCEPTIONS

using _error_i_base = interface_ptr<const error_base>;

class error_i : public _error_i_base, public std::exception {
public:
	error_i() = default;

	RUA_TMPL_FWD_CTOR(Args, _error_i_base, error_i)
	error_i(Args &&...args) : _error_i_base(std::forward<Args>(args)...) {}

	error_i(const error_i &src) :
		_error_i_base(static_cast<const _error_i_base &>(src)) {}

	error_i(error_i &&src) :
		_error_i_base(static_cast<_error_i_base &&>(std::move(src))) {}

	RUA_OVERLOAD_ASSIGNMENT(error_i)

	virtual ~error_i() = default;

	const char *what() const RUA_NOEXCEPT override {
		// I really don't like std::exception
		static thread_var<std::string> what_cache;
		return what_cache.emplace(get()->info()).c_str();
	}
};

#else

using error_i = interface_ptr<const error_base>;

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

	constexpr strv_error(string_view sv) : $sv(sv) {}

	virtual ~strv_error() = default;

	std::string info() const override {
		return std::string($sv);
	}

private:
	string_view $sv;
};

inline std::string to_string(const error_base &err) {
	return err.info();
}

inline std::string to_string(error_i err) {
	return err ? err->info() : "noerr";
}

RUA_CVAR const strv_error err_unimplemented("unimplemented");

////////////////////////////////////////////////////////////////////////////

#ifdef RUA_HAS_EXCEPTIONS

class exception_error : public error_base {
public:
	exception_error() = default;

	exception_error(std::exception_ptr ptr, std::string what) :
		$ptr(std::move(ptr)), $what(std::move(what)) {}

	virtual ~exception_error() = default;

	std::string info() const override {
		return $what;
	}

	void rethrow() {
		if (!$ptr) {
			return;
		}
		std::rethrow_exception(std::move($ptr));
		$ptr = nullptr;
		$what.clear();
	}

private:
	std::exception_ptr $ptr;
	std::string $what;
};

#endif

RUA_CVAR strv_error err_disabled_exceptions("disabled exceptions");
RUA_CVAR strv_error err_unthrowed_exception("unthrowed exception");
RUA_CVAR strv_error err_uncatched_exception("uncatched exception");

template <typename Exception>
inline error_i make_unthrow_exception_error(Exception &&e) {
#ifdef RUA_HAS_EXCEPTIONS
	std::string what(std::forward<Exception>(e).what());
	return exception_error(
		std::make_exception_ptr(std::forward<Exception>(e)), std::move(what));
#else
	return err_disabled_exceptions;
#endif
}

inline error_i current_exception_error(std::string what) {
#ifdef RUA_HAS_EXCEPTIONS
	auto ep = std::current_exception();
	if (!ep) {
		return err_unthrowed_exception;
	}
	return exception_error(std::move(ep), std::move(what));
#else
	return err_disabled_exceptions;
#endif
}

inline error_i current_exception_error(const std::exception &e) {
#ifdef RUA_HAS_EXCEPTIONS
	std::string what(e.what());
	return current_exception_error(std::move(what));
#else
	return err_disabled_exceptions;
#endif
}

inline error_i current_exception_error() {
#ifdef RUA_HAS_EXCEPTIONS
	auto ep = std::current_exception();
	if (!ep) {
		return err_unthrowed_exception;
	}
	try {
		std::rethrow_exception(ep);
	} catch (const std::exception &e) {
		return current_exception_error(e);
	}
	return err_uncatched_exception;
#else
	return err_disabled_exceptions;
#endif
}

////////////////////////////////////////////////////////////////////////////

RUA_CVAR strv_error unexpected("unexpected");

struct meet_expected_t {};

RUA_CVAL meet_expected_t meet_expected;

template <typename T = void>
class expected : public enable_value_operators<expected<T>, T> {
public:
	expected() : $v(unexpected) {}

	RUA_TMPL_FWD_CTOR(Args, RUA_A(variant<T, error_i>), expected)
	expected(Args &&...args) : $v(std::forward<Args>(args)...) {
		assert(has_value() || error());
	}

	RUA_TMPL_FWD_CTOR_IL(U, Args, T)
	expected(std::initializer_list<U> il, Args &&...args) :
		$v(in_place_type_t<T>{}, il, std::forward<Args>(args)...) {}

	expected(meet_expected_t) : $v(in_place_type_t<T>{}) {}

	expected(const expected &src) : $v(src.$v) {}

	expected(expected &&src) : $v(std::move(src.$v)) {}

	RUA_OVERLOAD_ASSIGNMENT(expected)

	bool has_value() const {
		return $v.template type_is<T>();
	}

	explicit operator bool() const {
		return has_value();
	}

	T &value() & {
#ifdef RUA_HAS_EXCEPTIONS
		if (!has_value()) {
			throw error();
		}
#endif
		assert(has_value());

		return $v.template as<T>();
	}

	T &&value() && {
#ifdef RUA_HAS_EXCEPTIONS
		if (!has_value()) {
			throw error();
		}
#endif
		assert(has_value());

		return std::move($v).template as<T>();
	}

	const T &value() const & {
#ifdef RUA_HAS_EXCEPTIONS
		if (!has_value()) {
			throw error();
		}
#endif
		assert(has_value());

		return $v.template as<T>();
	}

	error_i error() const {
		if ($v.template type_is<error_i>()) {
			const auto &err = $v.template as<error_i>();
			assert(err);
			return err;
		}
		assert(!$v.template type_is<void>());
		return nullptr;
	}

	variant<T, error_i> &vrt() & {
		return $v;
	}

	variant<T, error_i> &&vrt() && {
		return std::move($v);
	}

	const variant<T, error_i> &vrt() const & {
		return $v;
	}

	void reset() {
		$v.emplace(unexpected);
	}

	template <
		typename... Args,
		typename = enable_if_t<
			std::is_constructible<variant<T, error_i>, Args &&...>::value>>
	void emplace(Args &&...args) {
		$v.emplace(std::forward<Args>(args)...);
	}

	template <
		typename U,
		typename... Args,
		typename = enable_if_t<std::is_constructible<
			variant<T, error_i>,
			std::initializer_list<U>,
			Args &&...>::value>>
	void emplace(std::initializer_list<U> il, Args &&...args) {
		$v.emplace(il, std::forward<Args>(args)...);
	}

private:
	variant<T, error_i> $v;
};

template <>
class expected<void> {
public:
	expected() = default;

	template <
		typename Error,
		typename = enable_if_t<std::is_convertible<Error &&, error_i>::value>>
	expected(Error &&err) : $err(std::forward<Error>(err)) {}

	template <
		typename Variant,
		typename DecayVariant = decay_t<Variant>,
		typename = enable_if_t<
			is_variant<DecayVariant>::value &&
			std::is_convertible<Variant &&, variant<void, error_i>>::value>>
	expected(Variant &&v) {
		if (v.template type_is<void>()) {
			return;
		}
		auto r = std::forward<Variant>(v).visit([this](error_i err) {
			assert(err);
			$err = std::move(err);
		});
		assert(r);
	}

	expected(meet_expected_t) : $err() {}

	expected(const expected &src) : $err(src.$err) {}

	expected(expected &&src) : $err(std::move(src.$err)) {}

	RUA_OVERLOAD_ASSIGNMENT(expected)

	bool has_value() const {
		return !$err;
	}

	explicit operator bool() const {
		return has_value();
	}

	void value() const {
#ifdef RUA_HAS_EXCEPTIONS
		if (!has_value()) {
			throw error();
		}
#endif
	}

	error_i error() const {
		return $err;
	}

	variant<void, error_i> vrt() && {
		return $err ? variant<void, error_i>(std::move($err))
					: variant<void, error_i>();
	}

	variant<void, error_i> vrt() const & {
		return $err ? variant<void, error_i>($err) : variant<void, error_i>();
	}

	void reset() {
		$err.reset();
	}

	void emplace(error_i err) {
		$err = std::move(err);
	}

private:
	error_i $err;
};

////////////////////////////////////////////////////////////////////////////

template <typename T>
struct is_expected : std::false_type {};

template <typename T>
struct is_expected<expected<T>> : std::true_type {};

#ifdef RUA_HAS_INLINE_VAR
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
struct warp_expected<T, enable_if_t<std::is_base_of<T, error_i>::value>> {
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
try_invoke(F &&f, expected<T> exp) {
#ifdef RUA_HAS_EXCEPTIONS
	try {
#endif
		return rua::invoke(std::forward<F>(f), std::move(exp));
#ifdef RUA_HAS_EXCEPTIONS
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
try_invoke(F &&f, expected<T> exp) {
#ifdef RUA_HAS_EXCEPTIONS
	try {
#endif
		rua::invoke(std::forward<F>(f), std::move(exp));
#ifdef RUA_HAS_EXCEPTIONS
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
try_invoke(F &&f, expected<T> exp) {
#ifdef RUA_HAS_EXCEPTIONS
	try {
#endif
		return rua::invoke(std::forward<F>(f), expected<void>(exp.error()));
#ifdef RUA_HAS_EXCEPTIONS
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
try_invoke(F &&f, expected<T> exp) {
#ifdef RUA_HAS_EXCEPTIONS
	try {
#endif
		rua::invoke(std::forward<F>(f), expected<void>(exp.error()));
#ifdef RUA_HAS_EXCEPTIONS
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
try_invoke(F &&f, expected<T> exp) {
#ifdef RUA_HAS_EXCEPTIONS
	try {
#endif
		return rua::invoke(std::forward<F>(f), exp.error());
#ifdef RUA_HAS_EXCEPTIONS
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
try_invoke(F &&f, expected<T> exp) {
#ifdef RUA_HAS_EXCEPTIONS
	try {
#endif
		rua::invoke(std::forward<F>(f), exp.error());
#ifdef RUA_HAS_EXCEPTIONS
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
try_invoke(F &&f, expected<T> exp) {
	if (!exp) {
		return exp.error();
	}
#ifdef RUA_HAS_EXCEPTIONS
	try {
#endif
		return rua::invoke(std::forward<F>(f), std::move(exp).value());
#ifdef RUA_HAS_EXCEPTIONS
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
try_invoke(F &&f, expected<T> exp) {
	if (!exp) {
		return exp.error();
	}
#ifdef RUA_HAS_EXCEPTIONS
	try {
#endif
		rua::invoke(std::forward<F>(f), std::move(exp).value());
#ifdef RUA_HAS_EXCEPTIONS
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
try_invoke(F &&f, expected<T> exp) {
#ifdef RUA_HAS_EXCEPTIONS
	try {
#endif
		if (exp.has_value()) {
			return rua::invoke(std::forward<F>(f), std::move(exp).value());
		}
		return rua::invoke(std::forward<F>(f), exp.error());
#ifdef RUA_HAS_EXCEPTIONS
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
try_invoke(F &&f, expected<T> exp) {
#ifdef RUA_HAS_EXCEPTIONS
	try {
#endif
		if (exp.has_value()) {
			rua::invoke(std::forward<F>(f), std::move(exp).value());
		} else {
			rua::invoke(std::forward<F>(f), exp.error());
		}
#ifdef RUA_HAS_EXCEPTIONS
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
try_invoke(F &&f, expected<T> exp) {
	if (!exp) {
		return exp.error();
	}
#ifdef RUA_HAS_EXCEPTIONS
	try {
#endif
		return rua::invoke(std::forward<F>(f));
#ifdef RUA_HAS_EXCEPTIONS
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
try_invoke(F &&f, expected<T> exp) {
	if (!exp) {
		return exp.error();
	}
#ifdef RUA_HAS_EXCEPTIONS
	try {
#endif
		rua::invoke(std::forward<F>(f));
#ifdef RUA_HAS_EXCEPTIONS
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
inline decltype(try_invoke(std::declval<F &&>(), std::declval<Exp>()))
try_invoke(F &&f, T &&arg) {
	return try_invoke(std::forward<F>(f), Exp(std::forward<T>(arg)));
}

// INVOKE(F, GetVal() -> T => expected<T>) -> R => expected<R>
template <
	typename F,
	typename GetVal,
	typename... Args,
	typename Exp =
		warp_expected_t<decay_t<invoke_result_t<GetVal &&, Args &&...>>>>
inline decltype(try_invoke(std::declval<F &&>(), std::declval<Exp>()))
try_invoke(F &&f, GetVal &&get_val, Args &&...args) {
	Exp exp;
#ifdef RUA_HAS_EXCEPTIONS
	try {
#endif
		exp = rua::invoke(
			std::forward<GetVal>(get_val), std::forward<Args>(args)...);
#ifdef RUA_HAS_EXCEPTIONS
	} catch (const std::exception &e) {
		exp = current_exception_error(e);
	}
#endif
	return try_invoke(std::forward<F>(f), std::move(exp));
}

// INVOKE(F, void => expected<void>) -> R => expected<R>
template <typename F>
inline decltype(try_invoke(
	std::declval<F &&>(), std::declval<expected<void>>()))
try_invoke(F &&f) {
	return try_invoke(std::forward<F>(f), expected<void>());
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
