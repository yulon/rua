#ifndef _RUA_SPAN_HPP
#define _RUA_SPAN_HPP

#include "types/util.hpp"

#include <string>

namespace rua {

template <
	typename T,
	typename DataT = decltype(std::declval<T &&>().data()),
	typename Pointer = remove_reference_t<DataT>,
	typename Element = remove_pointer_t<Pointer>>
inline constexpr enable_if_t<
	std::is_pointer<Pointer>::value && !std::is_void<Element>::value &&
		!std::is_function<remove_cv_t<Element>>::value &&
		!is_null_pointer<Pointer>::value,
	DataT>
data(T &&target) {
	return std::forward<T>(target).data();
}

#if RUA_CPP < RUA_CPP_17

template <typename CharT, typename Traits, typename Allocator>
inline CharT *data(std::basic_string<CharT, Traits, Allocator> &target) {
	return &target[0];
}

template <typename CharT, typename Traits, typename Allocator>
inline CharT *data(std::basic_string<CharT, Traits, Allocator> &&target) {
	return data(target);
}

#endif

template <typename E, size_t N>
inline constexpr E *data(E (&c_ary_lv)[N]) {
	return &c_ary_lv[0];
}

template <typename E, size_t N>
inline constexpr E *data(E(&&c_ary_rv)[N]) {
	return &c_ary_rv[0];
}

template <typename E, size_t N>
inline constexpr E *data(E (*c_ary_ptr)[N]) {
	return &(*c_ary_ptr)[0];
}

template <typename, typename = void>
struct _has_data : std::false_type {};

template <typename T>
struct _has_data<T, void_t<decltype(std::declval<T>().data())>>
	: std::true_type {};

template <
	typename T,
	typename = enable_if_t<!_has_data<T &&>::value>,
	typename Pointer = decltype(std::declval<T &&>().begin()),
	typename Element = remove_pointer_t<Pointer>>
inline constexpr enable_if_t<
	std::is_pointer<Pointer>::value && !std::is_void<Element>::value &&
		!std::is_function<remove_cv_t<Element>>::value &&
		!is_null_pointer<Pointer>::value,
	Pointer>
data(T &&target) {
	return std::forward<T>(target).begin();
}

template <typename T>
inline constexpr decltype(std::declval<T &&>().size()) size(T &&target) {
	return std::forward<T>(target).size();
}

template <typename E, size_t N>
inline constexpr size_t size(E (&)[N]) {
	return N;
}

template <typename, typename = void>
struct _has_size : std::false_type {};

template <typename T>
struct _has_size<T, void_t<decltype(std::declval<T>().size())>>
	: std::true_type {};

template <typename T>
inline enable_if_t<
	!_has_size<T &&>::value,
	decltype(std::declval<T &&>().end() - std::declval<T &&>().begin())>
size(T &&target) {
	return std::forward<T>(target).end() - std::forward<T>(target).begin();
}

template <
	typename T,
	typename Pointer = remove_reference_t<decltype(data(std::declval<T>()))>,
	typename Element = remove_pointer_t<Pointer>,
	typename Size = remove_reference_t<decltype(size(std::declval<T>()))>>
struct span_traits {
	using pointer = Pointer;
	using element_type = Element;
	using value_type = remove_cv_t<Element>;
	using size_type = Size;
};

template <typename, typename = void>
struct is_span : std::false_type {};

template <typename T>
struct is_span<T, void_t<span_traits<T>>> : std::true_type {};

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T>
inline constexpr auto is_span_v = is_span<T>::value;
#endif

template <typename, typename = void>
struct is_readonly_span : std::false_type {};

template <typename T>
struct is_readonly_span<
	T,
	void_t<enable_if_t<
		std::is_const<typename span_traits<T>::element_type>::value>>>
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
	void_t<enable_if_t<
		!std::is_const<typename span_traits<T>::element_type>::value>>>
	: std::true_type {};

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T>
inline constexpr auto is_writeable_span_v = is_writeable_span<T>::value;
#endif

} // namespace rua

#endif
