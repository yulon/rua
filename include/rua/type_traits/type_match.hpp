#ifndef _RUA_TYPE_TRAITS_TYPE_MATCH_HPP
#define _RUA_TYPE_TRAITS_TYPE_MATCH_HPP

#include "rebind_if.hpp"
#include "std_patch.hpp"

namespace rua {

struct default_t {};

template <typename... Args>
struct type_match;

template <
	typename Expected,
	typename Comparison,
	typename ResultOfMatched,
	typename ResultOfUnmatched>
struct type_match<Expected, Comparison, ResultOfMatched, ResultOfUnmatched> {
	using is_matched = std::is_convertible<Comparison, Expected>;

	static constexpr bool is_matched_v = is_matched::value;

	using type =
		conditional_t<is_matched_v, ResultOfMatched, ResultOfUnmatched>;

	template <typename... Args>
	struct rebind {
		using type = type_match<Args...>;
	};
};

template <typename Expected, typename Comparison, typename ResultOfMatched>
struct type_match<Expected, Comparison, ResultOfMatched>
	: type_match<Expected, Comparison, ResultOfMatched, default_t> {};

template <typename Top, typename Expected, typename... Others>
using _type_match_multicase_handler =
	rebind_if<!Top::is_matched_v, Top, Expected, Others...>;

template <
	typename Expected,
	typename Comparison,
	typename ResultOfMatched,
	typename... Others>
struct type_match<Expected, Comparison, ResultOfMatched, Others...>
	: _type_match_multicase_handler<
		  type_match<Expected, Comparison, ResultOfMatched>,
		  Expected,
		  Others...>::type {};

template <typename... Args>
using type_match_t = typename type_match<Args...>::type;

template <typename... Args>
using type_switch = type_match<std::true_type, Args...>;

template <typename... Args>
using type_switch_t = typename type_switch<Args...>::type;

template <typename type_match_result>
struct _enable_matched : enable_if<
							 type_match_result::is_matched_v,
							 typename type_match_result::type> {};

template <typename... Args>
struct enable_matched : _enable_matched<type_match<Args...>> {};

template <typename... Args>
using enable_matched_t = typename enable_matched<Args...>::type;

template <typename... Args>
using enable_switched = enable_matched<std::true_type, Args...>;

template <typename... Args>
using enable_switched_t = typename enable_switched<Args...>::type;

} // namespace rua

#endif
