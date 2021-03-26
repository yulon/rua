#ifndef _RUA_CHECK_HPP
#define _RUA_CHECK_HPP

#include "log.hpp"
#include "string.hpp"
#include "types/info.hpp"

#include <cstdlib>

namespace rua {

#define _RUA_CHECK(_check, _lhs, _op, _rhs)                                    \
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
			eol::sys_con,
			"Address:",
			reinterpret_cast<void *>(&result),
			eol::sys_con,
			"Source:",
			str_join(file, ":", line),
			eol::sys_con,
			"Expression:",
			_check_hs_expr(lhs_expr, bracketed_lhs_expr),
			op,
			_check_hs_expr(rhs_expr, bracketed_rhs_expr),
			eol::sys_con,
			"LHS:",
			std::forward<LHS>(lhs),
			eol::sys_con,
			"RHS:",
			std::forward<RHS>(rhs),
			eol::sys_con,
			"Result:",
			std::forward<Result>(result));
		abort();
	}
}

#define RUA_CHECK(_lhs, _op, _rhs) _RUA_CHECK(rua::_check, _lhs, _op, _rhs)

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
			eol::sys_con,
			"Address:",
			reinterpret_cast<void *>(&result),
			eol::sys_con,
			"Source:",
			str_join(file, ":", line),
			eol::sys_con,
			"Expression:",
			_check_hs_expr(lhs_expr, bracketed_lhs_expr),
			op,
			_check_hs_expr(rhs_expr, bracketed_rhs_expr),
			eol::sys_con,
			"LHS:",
			str_join("<", type_name<LHS &&>::get(), ">"),
			std::forward<LHS>(lhs),
			eol::sys_con,
			"RHS:",
			str_join("<", type_name<RHS &&>::get(), ">"),
			std::forward<RHS>(rhs),
			eol::sys_con,
			"Result:",
			str_join("<", type_name<Result &&>::get(), ">"),
			std::forward<Result>(result));
		abort();
	}
}

#define RUA_CHECK_EX(_lhs, _op, _rhs)                                          \
	_RUA_CHECK(rua::_check_ex, _lhs, _op, _rhs)

#define _RUA_SUCCESS(_success, _expr)                                          \
	_success(_expr, #_expr, __FILE__, std::to_string(__LINE__))

template <typename Result>
inline void _success(
	Result &&result, string_view expr, string_view file, string_view line) {
	if (!static_cast<bool>(result)) {
		rua::err_log(
			"Check failed!",
			eol::sys_con,
			"Address:",
			reinterpret_cast<void *>(&result),
			eol::sys_con,
			"Source:",
			str_join(file, ":", line),
			eol::sys_con,
			"Expression:",
			expr,
			eol::sys_con,
			"Result:",
			std::forward<Result>(result));
		abort();
	}
}

#define RUA_SUCCESS(_expr) _RUA_SUCCESS(rua::_success, _expr)

template <typename Result>
inline void _success_ex(
	Result &&result, string_view expr, string_view file, string_view line) {
	if (!static_cast<bool>(result)) {
		rua::err_log(
			"Check failed!",
			eol::sys_con,
			"Address:",
			reinterpret_cast<void *>(&result),
			eol::sys_con,
			"Source:",
			str_join(file, ":", line),
			eol::sys_con,
			"Expression:",
			expr,
			eol::sys_con,
			"Result:",
			str_join("<", type_name<Result &&>::get(), ">"),
			std::forward<Result>(result));
		abort();
	}
}

#define RUA_SUCCESS_EX(_expr) _RUA_SUCCESS(rua::_success_ex, _expr)

#define RUA_FAIL(_expr) RUA_SUCCESS(!(_expr))

#define RUA_FAIL_EX(_expr) RUA_SUCCESS_EX(!(_expr))

} // namespace rua

#endif
