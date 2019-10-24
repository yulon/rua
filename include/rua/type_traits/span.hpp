#ifndef _RUA_TYPE_TRAITS_SPAN_HPP
#define _RUA_TYPE_TRAITS_SPAN_HPP

#include "std_patch.hpp"

#include "../byte.hpp"
#include "../generic_ptr.hpp"

#include <string>

namespace rua {

template <
	typename T,
	typename DecayT = decay_t<T>,
	typename = enable_if_t<std::is_class<DecayT>::value>,
	typename R = decltype(std::declval<T &&>().data()),
	typename Pointer = remove_reference_t<R>,
	typename = enable_if_t<
		std::is_pointer<Pointer>::value ||
		std::is_convertible<Pointer, void *>::value>>
RUA_FORCE_INLINE Pointer data(T &&target) {
	return std::forward<T>(target).data();
}

template <typename CharT, typename Traits, typename Allocator>
RUA_FORCE_INLINE CharT *
data(std::basic_string<CharT, Traits, Allocator> &target) {
#if RUA_CPP >= RUA_CPP_17
	return target.data();
#else
	return const_cast<CharT *>(target.data());
#endif
}

template <typename CharT, typename Traits, typename Allocator>
RUA_FORCE_INLINE CharT *
data(std::basic_string<CharT, Traits, Allocator> &&target) {
	return data(target);
}

template <typename T, size_t N>
RUA_FORCE_INLINE constexpr T *data(T (&inst)[N]) {
	return inst;
}

template <class E>
RUA_FORCE_INLINE constexpr const E *data(std::initializer_list<E> il) {
	return il.begin();
}

template <
	typename T,
	typename DecayT = decay_t<T>,
	typename = enable_if_t<std::is_class<DecayT>::value>,
	typename R = decltype(std::declval<T &&>().size()),
	typename DecayR = decay_t<R>,
	typename = enable_if_t<std::is_integral<DecayR>::value>>
RUA_FORCE_INLINE DecayR size(T &&target) {
	return std::forward<T>(target).size();
}

template <typename T, size_t N>
RUA_FORCE_INLINE constexpr size_t size(T (&)[N]) {
	return N;
}

template <
	typename T,
	typename Pointer = remove_reference_t<decltype(data(std::declval<T>()))>,
	typename Element = conditional_t<
		std::is_pointer<Pointer>::value,
		remove_pointer_t<Pointer>,
		byte>,
	typename Value = remove_cv_t<Element>,
	typename Index = remove_reference_t<decltype(size(std::declval<T>()))>>
struct span_traits {
	using pointer = Pointer;
	using element_type = Element;
	using value_type = Value;
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
	typename Pointer = remove_reference_t<decltype(data(std::declval<T>()))>,
	typename Element = conditional_t<
		std::is_pointer<Pointer>::value,
		remove_pointer_t<Pointer>,
		byte>>
struct span_element {
	using type = Element;
};

template <typename T>
using span_element_t = typename span_element<T>::type;

template <
	typename T,
	typename Pointer = remove_reference_t<decltype(data(std::declval<T>()))>,
	typename Element = conditional_t<
		std::is_pointer<Pointer>::value,
		remove_pointer_t<Pointer>,
		byte>,
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
