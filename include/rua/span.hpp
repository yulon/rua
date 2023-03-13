#ifndef _rua_span_hpp
#define _rua_span_hpp

#include "util.hpp"

#if RUA_CPP < RUA_CPP_17

#include <string>

#endif

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
inline constexpr E *data(E (&&c_ary_rv)[N]) {
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
inline constexpr enable_if_t<
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

#ifdef RUA_HAS_INLINE_VAR
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

#ifdef RUA_HAS_INLINE_VAR
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

#ifdef RUA_HAS_INLINE_VAR
template <typename T>
inline constexpr auto is_writeable_span_v = is_writeable_span<T>::value;
#endif

////////////////////////////////////////////////////////////////////////////

template <typename T>
class span {
public:
	constexpr span(std::nullptr_t = nullptr) : _p(nullptr), _n(0) {}

	constexpr span(T *ptr, size_t size) : _p(ptr), _n(ptr ? size : 0) {}

	constexpr span(T &val, size_t size = 1) : _p(&val), _n(size) {}

	constexpr span(T &&val, size_t size = 1) : _p(&val), _n(size) {}

	template <
		typename Span,
		typename SpanTraits = span_traits<Span &&>,
		typename = enable_if_t<
			!std::is_base_of<span, decay_t<Span>>::value &&
			std::is_same<typename SpanTraits::value_type, remove_cv_t<T>>::
				value &&
			(!std::is_const<typename SpanTraits::element_type>::value ||
			 std::is_const<T>::value)>>
	constexpr span(Span &&src) :
		span(data(std::forward<Span>(src)), size(std::forward<Span>(src))) {}

	constexpr explicit operator bool() const {
		return _n;
	}

	constexpr size_t size() const {
		return _n;
	}

	constexpr T *begin() const {
		return _p;
	}

	constexpr T *end() const {
		return _p + _n;
	}

	constexpr T &operator[](ptrdiff_t offset) const {
		return _p[offset];
	}

	constexpr span<T>
	slice(ptrdiff_t begin_offset, ptrdiff_t end_offset_from_begin) const {
		return begin_offset != end_offset_from_begin
				   ? span<T>(
						 _p + begin_offset,
						 static_cast<size_t>(
							 end_offset_from_begin - begin_offset))
				   : nullptr;
	}

	constexpr span<T> slice(ptrdiff_t begin_offset) const {
		return slice(begin_offset, size());
	}

	template <typename Span>
	constexpr span<T>
	operator()(ptrdiff_t begin_offset, ptrdiff_t end_offset_from_begin) const {
		return slice(begin_offset, end_offset_from_begin);
	}

	template <typename Span>
	constexpr span<T> operator()(ptrdiff_t begin_offset) const {
		return slice(begin_offset);
	}

	void resize(size_t size) {
		if (!size) {
			reset();
			return;
		}
		if (!_p) {
			return;
		}
		_n = size;
	}

	void reset() {
		if (!_p) {
			return;
		}
		_p = nullptr;
		_n = 0;
	}

	template <typename... Args>
	void reset(Args &&...args) {
		RUA_SASSERT((std::is_constructible<span<T>, Args...>::value));

		*this = span<T>(std::forward<Args>(args)...);
	}

private:
	T *_p;
	size_t _n;
};

template <typename E>
inline constexpr span<E> make_span(E *ptr, size_t size) {
	return span<E>(ptr, size);
}

template <typename E>
inline constexpr span<E> make_span(E &val, size_t size) {
	return span<E>(&val, size);
}

template <typename E>
inline constexpr span<E> make_span(E &&val, size_t size) {
	return span<E>(&val, size);
}

template <
	typename Span,
	typename SpanTraits = span_traits<Span &&>,
	typename E = typename SpanTraits::element_type>
inline constexpr span<E> make_span(Span &&src) {
	return span<E>(src);
}

} // namespace rua

#endif
