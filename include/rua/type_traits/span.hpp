#ifndef _RUA_TYPE_TRAITS_SPAN_HPP
#define _RUA_TYPE_TRAITS_SPAN_HPP

#include "std_patch.hpp"

#include "../stldata.hpp"

#include <type_traits>

namespace rua {

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

} // namespace rua

#endif
