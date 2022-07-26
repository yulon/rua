#ifndef _RUA_ERROR_HPP
#define _RUA_ERROR_HPP

#include "interface_ptr.hpp"
#include "string/char_set.hpp"
#include "string/conv.hpp"
#include "string/join.hpp"
#include "variant.hpp"

#include <cassert>
#include <list>

namespace rua {

class error_base;

using error_i = interface_ptr<error_base>;

class error_base {
public:
	error_base() = default;

	virtual std::string str() const = 0;

	error_base(error_i underlying_error) : _ue(std::move(underlying_error)) {}

	error_i underlying() const {
		return _ue;
	}

	std::string full_str(string_view sep = " <- ") const {
		if (!_ue) {
			return str();
		}
		std::list<std::string> strs;
		strs.emplace_back(str());
		auto e = _ue;
		do {
			strs.emplace_back(e->str());
			e = e->underlying();
		} while (e);
		return join(strs, sep);
	}

private:
	error_i _ue;
};

class string_error : public error_base {
public:
	string_error() = default;

	string_error(std::string s, error_i underlying_error = nullptr) :
		error_base(std::move(underlying_error)), _s(std::move(s)) {}

	std::string str() const override {
		return _s;
	}

private:
	std::string _s;
};

class unexpected_error : public error_base {
public:
	unexpected_error() = default;

	std::string str() const override {
		return "unexpected";
	}
};

inline std::string to_string(const error_base &err) {
	return err.full_str();
}

inline std::string to_string(error_i err) {
	return err ? err->full_str() : "noerr";
}

template <typename T>
class expected {
public:
	constexpr expected() = default;

	template <
		typename... Args,
		typename = enable_if_t<
			std::is_constructible<variant<T, error_i>, Args &&...>::value &&
			(sizeof...(Args) > 1 ||
			 !std::is_base_of<expected, decay_t<front_t<Args...>>>::value)>>
	constexpr expected(Args &&...args) : _val(std::forward<Args>(args)...) {}

	bool has_value() const {
		return _val.template type_is<T>();
	}

	explicit operator bool() const {
		return has_value();
	}

	T &value() & {
		assert(_val.template type_is<T>());

		return _val.template as<T>();
	}

	T &value() && {
		assert(_val.template type_is<T>());

		return std::move(_val).template as<T>();
	}

	const T &value() const & {
		assert(_val.template type_is<T>());

		return _val.template as<T>();
	}

	T &operator*() & {
		return value();
	}

	const T &operator*() const & {
		return value();
	}

	T &&operator*() && {
		return std::move(value());
	}

	T *operator->() {
		return &value();
	}

	const T *operator->() const {
		return &value();
	}

	error_i error() const {
		static unexpected_error ue;
		return _val.template type_is<error_i>()
				   ? _val.template as<error_i>()
				   : (has_value() ? error_i() : ue);
	}

	void reset() {
		_val.reset();
	}

private:
	variant<T, error_i> _val;
};

} // namespace rua

#endif
