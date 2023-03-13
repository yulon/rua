#ifndef _rua_range_traits_hpp
#define _rua_range_traits_hpp

#include "../span.hpp"
#include "../util.hpp"

#include <iterator>

namespace rua {

template <
	typename T,
	typename Iterator =
		remove_reference_t<decltype(std::begin(std::declval<T>()))>,
	typename Element = remove_reference_t<decltype(*std::declval<Iterator>())>>
struct range_traits {
	using iterator = Iterator;
	using element_type = Element;
	using value_type = remove_cv_t<Element>;
};

template <typename, typename = void>
struct is_range : std::false_type {};

template <typename T>
struct is_range<T, void_t<range_traits<T>>> : std::true_type {};

#ifdef RUA_HAS_INLINE_VAR
template <typename T>
inline constexpr auto is_range_v = is_range<T>::value;
#endif

template <typename, typename = void>
struct is_readonly_range : std::false_type {};

template <typename T>
struct is_readonly_range<
	T,
	void_t<enable_if_t<
		std::is_const<typename range_traits<T>::element_type>::value>>>
	: std::true_type {};

#ifdef RUA_HAS_INLINE_VAR
template <typename T>
inline constexpr auto is_readonly_range_v = is_readonly_range<T>::value;
#endif

template <typename, typename = void>
struct is_writeable_range : std::false_type {};

template <typename T>
struct is_writeable_range<
	T,
	void_t<enable_if_t<
		!std::is_const<typename range_traits<T>::element_type>::value>>>
	: std::true_type {};

#ifdef RUA_HAS_INLINE_VAR
template <typename T>
inline constexpr auto is_writeable_range_v = is_writeable_range<T>::value;
#endif

template <typename T>
inline constexpr decltype(size(std::declval<T &&>())) range_size(T &&target) {
	return size(std::forward<T>(target));
}

template <typename, typename = void>
struct _can_use_span_size : std::false_type {};

template <typename T>
struct _can_use_span_size<T, void_t<decltype(size(std::declval<T>()))>>
	: std::true_type {};

template <typename T>
inline constexpr enable_if_t<
	!_can_use_span_size<T &&>::value,
	decltype(std::distance(
		range_begin(std::declval<T &&>()), range_end(std::declval<T &&>())))>
range_size(T &&target) {
	return std::distance(
		range_begin(std::forward<T>(target)),
		range_end(std::forward<T>(target)));
}

} // namespace rua

#endif
