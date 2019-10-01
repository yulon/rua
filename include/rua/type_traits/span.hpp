#ifndef _RUA_TYPE_TRAITS_SPAN_HPP
#define _RUA_TYPE_TRAITS_SPAN_HPP

#include "std_patch.hpp"

#include "../stldata.hpp"

#include <type_traits>

namespace rua {

template <
	typename T,
	typename Pointer = typename std::remove_reference<decltype(
		stldata(std::declval<T &>()))>::type,
	typename Element = typename std::remove_pointer<Pointer>::type,
	typename =
		typename std::enable_if<!std::is_void<Element>::value, void>::type,
	typename Index = typename std::remove_reference<decltype(
		std::declval<T>().size())>::type,
	typename =
		typename std::enable_if<std::is_integral<Index>::value, void>::type>
struct get_span_element {
	using type = Element;
};

template <typename T>
using get_span_element_t = typename get_span_element<T>::type;

template <typename, typename = void>
struct is_span : std::false_type {};

template <typename T>
struct is_span<T, void_t<typename get_span_element<T>::type>> : std::true_type {
};

template <typename T>
using is_span_t = typename is_span<T>::type;

template <typename, typename = void>
struct is_readonly_span : std::false_type {};

template <typename T>
struct is_readonly_span<
	T,
	void_t<typename std::enable_if<
		std::is_const<typename get_span_element<T>::type>::value>::type>>
	: std::true_type {};

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T>
inline constexpr auto is_readonly_span_v = is_readonly_span<T>::value;
#endif

template <typename, typename = void>
struct is_writeable_span : std::false_type {};

template <typename T>
struct is_writeable_span<
	T,
	void_t<typename std::enable_if<
		!std::is_const<typename get_span_element<T>::type>::value>::type>>
	: std::true_type {};

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T>
inline constexpr auto is_writeable_span_v = is_writeable_span<T>::value;
#endif

} // namespace rua

#endif
