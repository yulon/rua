#ifndef _RUA_SPAN_HPP
#define _RUA_SPAN_HPP

#include "generic_ptr.hpp"
#include "types/traits.hpp"
#include "types/util.hpp"

#include <string>

namespace rua {

template <
	typename T,
	typename DataT = decltype(std::declval<T &&>().data()),
	typename Pointer = remove_reference_t<DataT>>
RUA_FORCE_INLINE constexpr enable_if_t<
	std::is_pointer<Pointer>::value &&
		!std::is_void<remove_pointer_t<Pointer>>::value &&
		!is_null_pointer<Pointer>::value,
	DataT>
data(T &&target) {
	return std::forward<T>(target).data();
}

template <
	typename T,
	typename DataT = decltype(std::declval<T &&>().data()),
	typename Pointer = remove_reference_t<DataT>,
	typename Element = remove_pointer_t<Pointer>,
	typename BytePointer = enable_if_t<
		std::is_void<Element>::value || is_null_pointer<Pointer>::value,
		conditional_t<std::is_const<Element>::value, const byte *, byte *>>>
RUA_FORCE_INLINE BytePointer data(T &&target) {
	return reinterpret_cast<BytePointer>(std::forward<T>(target).data());
}

template <
	typename T,
	typename DataT = decltype(std::declval<T &&>().data()),
	typename Pointer = remove_reference_t<DataT>,
	bool IsConst = std::is_const<remove_reference_t<T>>::value,
	typename VoidPointer = conditional_t<IsConst, const void *, void *>,
	typename BytePointer = conditional_t<IsConst, const byte *, byte *>>
RUA_FORCE_INLINE enable_if_t<
	!std::is_pointer<Pointer>::value &&
		std::is_convertible<DataT, VoidPointer>::value &&
		!std::is_convertible<DataT, BytePointer>::value,
	BytePointer>
data(T &&target) {
	return reinterpret_cast<BytePointer>(
		static_cast<VoidPointer>(std::forward<T>(target).data()));
}

template <
	typename T,
	typename DataT = decltype(std::declval<T &&>().data()),
	typename Pointer = remove_reference_t<DataT>,
	bool IsConst = std::is_const<remove_reference_t<T>>::value,
	typename VoidPointer = conditional_t<IsConst, const void *, void *>,
	typename BytePointer = conditional_t<IsConst, const byte *, byte *>>
RUA_FORCE_INLINE constexpr enable_if_t<
	!std::is_pointer<Pointer>::value &&
		std::is_convertible<DataT, BytePointer>::value,
	BytePointer>
data(T &&target) {
	return static_cast<BytePointer>(std::forward<T>(target).data());
}

#if RUA_CPP < RUA_CPP_17

template <typename CharT, typename Traits, typename Allocator>
RUA_FORCE_INLINE CharT *
data(std::basic_string<CharT, Traits, Allocator> &target) {
	return &target[0];
}

template <typename CharT, typename Traits, typename Allocator>
RUA_FORCE_INLINE CharT *
data(std::basic_string<CharT, Traits, Allocator> &&target) {
	return data(target);
}

#endif

template <typename E, size_t N>
RUA_FORCE_INLINE constexpr E *data(E (&c_ary_lv)[N]) {
	return &c_ary_lv[0];
}

template <typename E, size_t N>
RUA_FORCE_INLINE constexpr E *data(E(&&c_ary_rv)[N]) {
	return &c_ary_rv[0];
}

template <typename E, size_t N>
RUA_FORCE_INLINE constexpr E *data(E (*c_ary_ptr)[N]) {
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
	typename Pointer = decltype(std::declval<T &&>().begin())>
RUA_FORCE_INLINE constexpr enable_if_t<
	std::is_pointer<Pointer>::value &&
		!std::is_void<remove_pointer_t<Pointer>>::value &&
		!is_null_pointer<Pointer>::value,
	Pointer>
data(T &&target) {
	return std::forward<T>(target).begin();
}

template <typename T>
RUA_FORCE_INLINE constexpr decltype(std::declval<T &&>().size())
size(T &&target) {
	return std::forward<T>(target).size();
}

template <typename E, size_t N>
RUA_FORCE_INLINE constexpr size_t size(E (&)[N]) {
	return N;
}

template <typename, typename = void>
struct _has_size : std::false_type {};

template <typename T>
struct _has_size<T, void_t<decltype(std::declval<T>().size())>>
	: std::true_type {};

template <typename T>
RUA_FORCE_INLINE enable_if_t<
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

template <typename T>
using is_span_t = typename is_span<T>::type;

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
