#ifndef _RUA_TYPE_TRAITS_SPAN_HPP
#define _RUA_TYPE_TRAITS_SPAN_HPP

#include "std_patch.hpp"

#include <string>
#include <type_traits>

namespace rua {

template <
	typename T,
	typename DecayT = typename std::decay<T>::type,
	typename = typename std::enable_if<std::is_class<DecayT>::value>::type,
	typename R = decltype(std::declval<T &&>().data()),
	typename Pointer = typename std::remove_reference<R>::type,
	typename = typename std::enable_if<
		std::is_pointer<Pointer>::value &&
		!std::is_null_pointer<Pointer>::value>::type,
	typename Element = typename std::remove_pointer<Pointer>::type,
	typename =
		typename std::enable_if<!std::is_void<Element>::value, void>::type>
RUA_FORCE_INLINE Pointer span_data_of(T &&target) {
	return std::forward<T>(target).data();
}

template <typename CharT, typename Traits, typename Allocator>
RUA_FORCE_INLINE CharT *
span_data_of(std::basic_string<CharT, Traits, Allocator> &target) {
#if RUA_CPP >= RUA_CPP_17
	return target.data();
#else
	return const_cast<CharT *>(target.data());
#endif
}

template <typename CharT, typename Traits, typename Allocator>
RUA_FORCE_INLINE CharT *
span_data_of(std::basic_string<CharT, Traits, Allocator> &&target) {
	return span_data_of(target);
}

template <typename T, size_t N>
RUA_FORCE_INLINE constexpr T *span_data_of(T (&inst)[N]) {
	return inst;
}

template <
	typename T,
	typename DecayT = typename std::decay<T>::type,
	typename = typename std::enable_if<std::is_class<DecayT>::value>::type,
	typename R = decltype(std::declval<T &&>().size()),
	typename DecayR = typename std::decay<R>::type,
	typename = typename std::enable_if<std::is_integral<DecayR>::value>::type>
RUA_FORCE_INLINE DecayR span_size_of(T &&target) {
	return std::forward<T>(target).size();
}

template <typename T, size_t N>
RUA_FORCE_INLINE constexpr size_t span_size_of(T (&)[N]) {
	return N;
}

template <
	typename T,
	typename Pointer = typename std::remove_reference<decltype(
		span_data_of(std::declval<T>()))>::type,
	typename Element = typename std::remove_pointer<Pointer>::type,
	typename Index = typename std::remove_reference<decltype(
		span_size_of(std::declval<T>()))>::type>
struct span_traits {
	using pointer_type = Pointer;
	using element_type = Element;
	using index_type = Index;
};

template <typename, typename = void>
struct is_span : std::false_type {};

template <typename T>
struct is_span<T, void_t<span_traits<T>>> : std::true_type {};

template <typename T>
using is_span_t = typename is_span<T>::type;

template <typename, typename = void>
struct is_readonly_span : std::false_type {};

template <typename T>
struct is_readonly_span<
	T,
	void_t<typename std::enable_if<
		std::is_const<typename span_traits<T>::element_type>::value>::type>>
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
		!std::is_const<typename span_traits<T>::element_type>::value>::type>>
	: std::true_type {};

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T>
inline constexpr auto is_writeable_span_v = is_writeable_span<T>::value;
#endif

} // namespace rua

#endif
