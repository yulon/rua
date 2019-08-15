#ifndef _RUA_TYPE_TRAITS_HPP
#define _RUA_TYPE_TRAITS_HPP

#include "macros.hpp"
#include "stldata.hpp"

#include <cstddef>
#include <string>
#include <type_traits>

namespace rua {

// Compatibility and improvement of std::type_traits.

#ifdef __cpp_lib_bool_constant
template <bool Cond>
using bool_constant = std::bool_constant<Cond>;
#else
template <bool Cond>
using bool_constant = std::integral_constant<bool, Cond>;
#endif

template <bool Cond, typename... T>
struct enable_if;

template <typename T>
struct enable_if<true, T> {
	using type = T;
};

template <typename T, typename... TArgs>
struct enable_if<true, T, TArgs...> {
	using type = typename T::template rebind<TArgs...>::type;
};

template <typename... T>
struct enable_if<false, T...> {};

template <bool Cond, typename T, typename... F>
struct conditional;

template <typename T, typename... F>
struct conditional<true, T, F...> {
	using type = T;
};

template <typename T, typename F>
struct conditional<false, T, F> {
	using type = F;
};

template <typename T, typename F, typename... FArgs>
struct conditional<false, T, F, FArgs...> {
	using type = typename F::template rebind<FArgs...>::type;
};

////////////////////////////////////////////////////////////////////////

template <bool Cond, typename T, typename... Args>
struct rebind_if;

template <typename T, typename... Args>
struct rebind_if<true, T, Args...> {
	using type = typename T::template rebind<Args...>::type;
};

template <typename T, typename... Args>
struct rebind_if<false, T, Args...> {
	using type = T;
};

////////////////////////////////////////////////////////////////////////

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

	using type = typename std::
		conditional<is_matched_v, ResultOfMatched, ResultOfUnmatched>::type;

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

////////////////////////////////////////////////////////////////////////

template <typename type_match_result>
struct _enable_matched : std::enable_if<
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

////////////////////////////////////////////////////////////////////////

template <typename T>
inline std::string &type_info() {
	static std::string inf;
	return inf;
}

using type_id_t = std::string &(*)();

template <typename T>
constexpr type_id_t type_id() {
	return &type_info<T>;
}

inline std::string &type_info(type_id_t id) {
	return id();
}

////////////////////////////////////////////////////////////////////////

#define RUA_CONTAINER_OF(member_ptr, type, member)                             \
	reinterpret_cast<type *>(                                                  \
		reinterpret_cast<uintptr_t>(member_ptr) - offsetof(type, member))

#define RUA_IS_BASE_OF_CONCEPT(_B, _D)                                         \
	typename _D, typename = typename std::enable_if<std::is_base_of<           \
					 _B,                                                       \
					 typename std::remove_reference<_D>::type>::value>::type

#define RUA_DERIVED_CONCEPT(_B, _D)                                            \
	typename _D,                                                               \
		typename = typename std::enable_if <                                   \
					   std::is_base_of<                                        \
						   _B,                                                 \
						   typename std::remove_reference<_D>::type>::value && \
				   !std::is_same<_B, _D>::value > ::type

////////////////////////////////////////////////////////////////////////

template <bool IsCopyable, bool IsMovable>
class enable_copy_move;

template <>
class enable_copy_move<true, true> {
public:
	constexpr enable_copy_move() = default;

	enable_copy_move(const enable_copy_move &) = default;
	enable_copy_move(enable_copy_move &&) = default;

	RUA_OVERLOAD_ASSIGNMENT(enable_copy_move)
};

template <>
class enable_copy_move<true, false> {
public:
	constexpr enable_copy_move() = default;

	enable_copy_move(const enable_copy_move &) = default;

	RUA_OVERLOAD_ASSIGNMENT_L(enable_copy_move)
};

template <>
class enable_copy_move<false, true> {
public:
	constexpr enable_copy_move() = default;

	enable_copy_move(enable_copy_move &&) = default;

	RUA_OVERLOAD_ASSIGNMENT_R(enable_copy_move)
};

template <>
class enable_copy_move<false, false> {
public:
	constexpr enable_copy_move() = default;

	enable_copy_move(const enable_copy_move &) = delete;
	enable_copy_move(enable_copy_move &&) = delete;

	enable_copy_move &operator=(const enable_copy_move &) = delete;
	enable_copy_move &operator=(enable_copy_move &&) = delete;
};

template <typename T>
using enable_copy_move_from = enable_copy_move<
	std::is_copy_constructible<T>::value,
	std::is_move_constructible<T>::value>;

////////////////////////////////////////////////////////////////////////

template <typename T>
struct type_identity {
	using type = T;
};

template <typename T>
using type_identity_t = typename type_identity<T>::type;

////////////////////////////////////////////////////////////////////////

struct _secondary_t {};
struct _priority_t : _secondary_t {};

template <typename T>
inline constexpr void _get_span_element_type(_secondary_t);

template <
	typename T,
	typename pointer = decltype(stldata(std::declval<T &>())),
	typename element_type = typename std::remove_pointer<pointer>::type,
	typename index_type = decltype(std::declval<T>().size()),
	typename = typename std::
		enable_if<std::is_integral<index_type>::value, void>::type>
inline constexpr type_identity<element_type>
	_get_span_element_type(_priority_t);

template <typename T>
using get_span_element = decltype(_get_span_element_type<T>(_priority_t{}));

template <typename T>
using get_span_element_t = typename get_span_element<T>::type;

template <typename T>
struct is_span
	: bool_constant<!std::is_same<get_span_element<T>, void>::value> {};

template <typename T>
using is_span_t = typename is_span<T>::type;

template <typename T>
struct _is_non_void_and_mbr_type_is_const
	: bool_constant<
		  !std::is_same<T, void>::value &&
		  std::is_const<typename T::type>::value> {};

template <typename T>
struct is_readonly_span
	: _is_non_void_and_mbr_type_is_const<get_span_element<T>> {};

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T>
inline constexpr auto is_readonly_span_v = is_readonly_span<T>::value;
#endif

template <typename T>
struct _is_non_void_and_mbr_type_is_non_const
	: bool_constant<
		  !std::is_same<T, void>::value &&
		  !std::is_const<typename T::type>::value> {};

template <typename T>
struct is_writeable_span
	: _is_non_void_and_mbr_type_is_non_const<get_span_element<T>> {};

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T>
inline constexpr auto is_writeable_span_v = is_writeable_span<T>::value;
#endif

////////////////////////////////////////////////////////////////////////

template <typename FirstArg, typename... Args>
struct argments_front {
	using type = FirstArg;
};

template <typename... Args>
using argments_front_t = typename argments_front<Args...>::type;

} // namespace rua

#endif
