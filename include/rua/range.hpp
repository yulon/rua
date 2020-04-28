#ifndef _RUA_RANGE_HPP
#define _RUA_RANGE_HPP

#include "span.hpp"
#include "types/traits.hpp"
#include "types/util.hpp"

namespace rua {

template <
	typename T,
	typename BeginT = decltype(std::declval<T &&>().begin()),
	typename = decltype(*std::declval<BeginT>())>
inline constexpr BeginT begin(T &&target) {
	return std::forward<T>(target).begin();
}

template <
	typename T,
	typename EndT = decltype(std::declval<T &&>().end()),
	typename = decltype(*std::declval<EndT>())>
inline constexpr EndT end(T &&target) {
	return std::forward<T>(target).end();
}

template <typename, typename = void>
struct _has_begin_end : std::false_type {};

template <typename T>
struct _has_begin_end<
	T,
	void_t<
		decltype(std::declval<T>().begin()),
		decltype(std::declval<T>().end())>> : std::true_type {};

template <typename T>
inline constexpr enable_if_t<
	!_has_begin_end<T &&>::value,
	typename span_traits<T &&>::pointer>
begin(T &&target) {
	return data(std::forward<T>(target));
}

template <typename T>
inline constexpr enable_if_t<
	!_has_begin_end<T &&>::value,
	typename span_traits<T &&>::pointer>
end(T &&target) {
	return data(std::forward<T>(target)) + size(std::forward<T>(target));
}

} // namespace rua

namespace _rua_range_adl {

template <
	typename T,
	typename BeginT = decltype(begin(std::declval<T &&>())),
	typename = decltype(*std::declval<BeginT>())>
inline constexpr rua::enable_if_t<
	!rua::_has_begin_end<T &&>::value && !rua::is_span<T &&>::value,
	BeginT>
_begin(T &&target) {
	return begin(std::forward<T>(target));
}

template <
	typename T,
	typename EndT = decltype(end(std::declval<T &&>())),
	typename = decltype(*std::declval<EndT>())>
inline constexpr rua::enable_if_t<
	!rua::_has_begin_end<T &&>::value && !rua::is_span<T &&>::value,
	EndT>
_end(T &&target) {
	return end(std::forward<T>(target));
}

} // namespace _rua_range_adl

namespace rua {

template <typename T>
inline constexpr decltype(_rua_range_adl::_begin(std::declval<T &&>()))
begin(T &&target) {
	return _rua_range_adl::_begin(std::forward<T>(target));
}

template <typename T>
inline constexpr decltype(_rua_range_adl::_end(std::declval<T &&>()))
end(T &&target) {
	return _rua_range_adl::_end(std::forward<T>(target));
}

template <
	typename T,
	typename Iterator = remove_reference_t<decltype(begin(std::declval<T>()))>,
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

template <typename T>
using is_range_t = typename is_range<T>::type;

template <typename, typename = void>
struct is_readonly_range : std::false_type {};

template <typename T>
struct is_readonly_range<
	T,
	void_t<enable_if_t<
		std::is_const<typename range_traits<T>::element_type>::value>>>
	: std::true_type {};

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
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

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T>
inline constexpr auto is_writeable_range_v = is_writeable_range<T>::value;
#endif

} // namespace rua

#endif