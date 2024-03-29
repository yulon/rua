#ifndef _rua_check_hpp
#define _rua_check_hpp

#include "dype/type_info.hpp"
#include "log.hpp"
#include "string.hpp"
#include "util.hpp"

#include <cstdlib>

namespace rua {

#define _rua_check(_check, _lhs, _op, _rhs)                                    \
	_check(                                                                    \
		(_lhs)_op(_rhs),                                                       \
		_lhs,                                                                  \
		#_lhs,                                                                 \
		"(" #_lhs ")",                                                         \
		#_op,                                                                  \
		_rhs,                                                                  \
		#_rhs,                                                                 \
		"(" #_rhs ")",                                                         \
		__FILE__,                                                              \
		std::to_string(__LINE__))

inline constexpr string_view
_check_hs_expr(string_view hs_expr, string_view bracketed_hs_expr) {
	return hs_expr.find('=') != string_view::npos ||
				   hs_expr.find('>') != string_view::npos ||
				   hs_expr.find('<') != string_view::npos
			   ? bracketed_hs_expr
			   : hs_expr;
}

template <typename Result, typename LHS, typename RHS>
inline void _check(
	Result &&result,
	LHS &&lhs,
	string_view lhs_expr,
	string_view bracketed_lhs_expr,
	string_view op,
	RHS &&rhs,
	string_view rhs_expr,
	string_view bracketed_rhs_expr,
	string_view file,
	string_view line) {
	if (!static_cast<bool>(result)) {
		err_log(
			"Check failed!",
			eol::sys,
			"Address:",
			reinterpret_cast<const void *>(&result),
			eol::sys,
			"Source:",
			join(file, ":", line),
			eol::sys,
			"Expression:",
			_check_hs_expr(lhs_expr, bracketed_lhs_expr),
			op,
			_check_hs_expr(rhs_expr, bracketed_rhs_expr),
			eol::sys,
			"LHS:",
			std::forward<LHS>(lhs),
			eol::sys,
			"RHS:",
			std::forward<RHS>(rhs),
			eol::sys,
			"Result:",
			std::forward<Result>(result));
		abort();
	}
}

#define RUA_CHECK(_lhs, _op, _rhs) _rua_check(rua::_check, _lhs, _op, _rhs)

template <typename Result, typename LHS, typename RHS>
inline void _check_ex(
	Result &&result,
	LHS &&lhs,
	string_view lhs_expr,
	string_view bracketed_lhs_expr,
	string_view op,
	RHS &&rhs,
	string_view rhs_expr,
	string_view bracketed_rhs_expr,
	string_view file,
	string_view line) {
	if (!static_cast<bool>(result)) {
		err_log(
			"Check failed!",
			eol::sys,
			"Address:",
			reinterpret_cast<const void *>(&result),
			eol::sys,
			"Source:",
			join(file, ":", line),
			eol::sys,
			"Expression:",
			_check_hs_expr(lhs_expr, bracketed_lhs_expr),
			op,
			_check_hs_expr(rhs_expr, bracketed_rhs_expr),
			eol::sys,
			"LHS:",
			join("<", type_name<LHS &&>(), ">"),
			std::forward<LHS>(lhs),
			eol::sys,
			"RHS:",
			join("<", type_name<RHS &&>(), ">"),
			std::forward<RHS>(rhs),
			eol::sys,
			"Result:",
			join("<", type_name<Result &&>(), ">"),
			std::forward<Result>(result));
		abort();
	}
}

#define RUA_CHECK_EX(_lhs, _op, _rhs)                                          \
	_rua_check(rua::_check_ex, _lhs, _op, _rhs)

#define _rua_success(_success, _expr)                                          \
	_success(_expr, #_expr, __FILE__, std::to_string(__LINE__))

template <typename Result>
inline void _success(
	Result &&result, string_view expr, string_view file, string_view line) {
	if (!static_cast<bool>(result)) {
		rua::err_log(
			"Check failed!",
			eol::sys,
			"Address:",
			reinterpret_cast<const void *>(&result),
			eol::sys,
			"Source:",
			join(file, ":", line),
			eol::sys,
			"Expression:",
			expr,
			eol::sys,
			"Result:",
			std::forward<Result>(result));
		abort();
	}
}

#define RUA_SUCCESS(_expr) _rua_success(rua::_success, _expr)

template <typename Result>
inline void _success_ex(
	Result &&result, string_view expr, string_view file, string_view line) {
	if (!static_cast<bool>(result)) {
		rua::err_log(
			"Check failed!",
			eol::sys,
			"Address:",
			reinterpret_cast<const void *>(&result),
			eol::sys,
			"Source:",
			join(file, ":", line),
			eol::sys,
			"Expression:",
			expr,
			eol::sys,
			"Result:",
			join("<", type_name<Result &&>(), ">"),
			std::forward<Result>(result));
		abort();
	}
}

#define RUA_SUCCESS_EX(_expr) _rua_success(rua::_success_ex, _expr)

#define RUA_FAIL(_expr) RUA_SUCCESS(!(_expr))

#define RUA_FAIL_EX(_expr) RUA_SUCCESS_EX(!(_expr))

} // namespace rua

#endif
