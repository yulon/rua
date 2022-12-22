#ifndef _RUA_ERROR_HPP
#define _RUA_ERROR_HPP

#include "interface_ptr.hpp"
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

#else

using error_i = interface_ptr<error_base>;

#endif

class str_error : public error_base {
public:
	str_error() = default;

	str_error(std::string s) : _s(std::move(s)) {}

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
			if (err) {
				return err;
			}
		} else if (has_value()) {
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

	template <
		typename Error,
		typename = enable_if_t<std::is_constructible<error_i, Error &&>::value>>
	expected(Error &&err) : _err(std::forward<Error>(err)) {}

	expected(variant<error_i> vrt) :
		_err(vrt.type_is<error_i>() ? vrt.as<error_i>() : nullptr) {}

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
