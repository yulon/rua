#ifndef _RUA_TYPE_TRAITS_SPAN_HPP
#define _RUA_TYPE_TRAITS_SPAN_HPP

#include "std_patch.hpp"

#include "../byte.hpp"
#include "../generic_ptr.hpp"

#include <string>

namespace rua {

template <
	typename T,
	typename = enable_if_t<std::is_class<decay_t<T>>::value>,
	typename Pointer = decltype(std::declval<T &&>().data())>
RUA_FORCE_INLINE enable_if_t<
	std::is_convertible<Pointer, generic_ptr>::value &&
		!std::is_null_pointer<decay_t<Pointer>>::value,
	Pointer>
data(T &&target) {
	return std::forward<T>(target).data();
}

#if RUA_CPP < RUA_CPP_17

template <typename CharT, typename Traits, typename Allocator>
RUA_FORCE_INLINE CharT *
data(std::basic_string<CharT, Traits, Allocator> &target) {
	return const_cast<CharT *>(target.data());
}

template <typename CharT, typename Traits, typename Allocator>
RUA_FORCE_INLINE CharT *
data(std::basic_string<CharT, Traits, Allocator> &&target) {
	return data(target);
}

#endif

template <typename E, size_t N>
RUA_FORCE_INLINE constexpr E *data(E (&c_ary)[N]) {
	return c_ary;
}

template <class E>
RUA_FORCE_INLINE RUA_CONSTEXPR_14 const E *data(std::initializer_list<E> il) {
	return il.begin();
}

template <
	typename T,
	typename = enable_if_t<std::is_class<decay_t<T>>::value>,
	typename Index = decltype(std::declval<T &&>().size())>
RUA_FORCE_INLINE enable_if_t<
	std::is_integral<remove_reference_t<Index>>::value ||
		std::is_convertible<Index, size_t>::value,
	Index>
size(T &&target) {
	return std::forward<T>(target).size();
}

template <typename E, size_t N>
RUA_FORCE_INLINE constexpr size_t size(E (&)[N]) {
	return N;
}

template <
	typename T,
	typename Pointer = remove_reference_t<decltype(data(std::declval<T>()))>,
	typename Element = conditional_t<
		std::is_pointer<Pointer>::value,
		remove_pointer_t<Pointer>,
		conditional_t<std::is_const<decay_t<T>>::value, const byte, byte>>,
	typename Value = remove_cv_t<Element>,
	typename DecayElement = conditional_t<
		std::is_void<Value>::value,
		conditional_t<std::is_const<Element>::value, const byte, byte>,
		Element>,
	typename DecayValue = remove_cv_t<DecayElement>,
	typename Index = remove_reference_t<decltype(size(std::declval<T>()))>>
struct span_traits {
	using pointer = Pointer;
	using element_type = DecayElement;
	using value_type = DecayValue;
	using index_type = Index;
};

template <
	typename T,
	typename Pointer = remove_reference_t<decltype(data(std::declval<T>()))>>
struct span_pointer {
	using type = Pointer;
};

template <typename T>
using span_pointer_t = typename span_pointer<T>::type;

template <
	typename T,
	typename Pointer = span_pointer_t<T>,
	typename Element = conditional_t<
		std::is_pointer<Pointer>::value,
		remove_pointer_t<Pointer>,
		conditional_t<std::is_const<decay_t<T>>::value, const byte, byte>>,
	typename Value = remove_cv_t<Element>,
	typename DecayElement = conditional_t<
		std::is_void<Value>::value,
		conditional_t<std::is_const<Element>::value, const byte, byte>,
		Element>>
struct span_element {
	using type = DecayElement;
};

template <typename T>
using span_element_t = typename span_element<T>::type;

template <
	typename T,
	typename Element = span_element_t<T>,
	typename Value = remove_cv_t<Element>>
struct span_value {
	using type = Value;
};

template <typename T>
using span_value_t = typename span_value<T>::type;

template <
	typename T,
	typename Index = remove_reference_t<decltype(size(std::declval<T>()))>>
struct span_index {
	using type = Index;
};

template <typename T>
using span_index_t = typename span_index<T>::type;

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
