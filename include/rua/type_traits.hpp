#ifndef _RUA_TYPE_TRAITS_HPP
#define _RUA_TYPE_TRAITS_HPP

#include "any_word.hpp"
#include "macros.hpp"
#include "stldata.hpp"

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>

#ifdef RUA_USING_RTTI
#include <typeindex>
#include <typeinfo>
#endif

namespace rua {

// Compatibility and improvement of std::type_traits.

#ifdef __cpp_lib_bool_constant
template <bool Cond>
using bool_constant = std::bool_constant<Cond>;
#else
template <bool Cond>
using bool_constant = std::integral_constant<bool, Cond>;
#endif

template <bool Cond, typename... T>
struct enable_if;

template <typename T>
struct enable_if<true, T> {
	using type = T;
};

template <typename T, typename... TArgs>
struct enable_if<true, T, TArgs...> {
	using type = typename T::template rebind<TArgs...>::type;
};

template <typename... T>
struct enable_if<false, T...> {};

template <bool Cond, typename T, typename... F>
struct conditional;

template <typename T, typename... F>
struct conditional<true, T, F...> {
	using type = T;
};

template <typename T, typename F>
struct conditional<false, T, F> {
	using type = F;
};

template <typename T, typename F, typename... FArgs>
struct conditional<false, T, F, FArgs...> {
	using type = typename F::template rebind<FArgs...>::type;
};

////////////////////////////////////////////////////////////////////////

template <bool Cond, typename T, typename... Args>
struct rebind_if;

template <typename T, typename... Args>
struct rebind_if<true, T, Args...> {
	using type = typename T::template rebind<Args...>::type;
};

template <typename T, typename... Args>
struct rebind_if<false, T, Args...> {
	using type = T;
};

////////////////////////////////////////////////////////////////////////

struct default_t {};

template <typename... Args>
struct type_match;

template <
	typename Expected,
	typename Comparison,
	typename ResultOfMatched,
	typename ResultOfUnmatched>
struct type_match<Expected, Comparison, ResultOfMatched, ResultOfUnmatched> {
	using is_matched = std::is_convertible<Comparison, Expected>;

	static constexpr bool is_matched_v = is_matched::value;

	using type = typename std::
		conditional<is_matched_v, ResultOfMatched, ResultOfUnmatched>::type;

	template <typename... Args>
	struct rebind {
		using type = type_match<Args...>;
	};
};

template <typename Expected, typename Comparison, typename ResultOfMatched>
struct type_match<Expected, Comparison, ResultOfMatched>
	: type_match<Expected, Comparison, ResultOfMatched, default_t> {};

template <typename Top, typename Expected, typename... Others>
using _type_match_multicase_handler =
	rebind_if<!Top::is_matched_v, Top, Expected, Others...>;

template <
	typename Expected,
	typename Comparison,
	typename ResultOfMatched,
	typename... Others>
struct type_match<Expected, Comparison, ResultOfMatched, Others...>
	: _type_match_multicase_handler<
		  type_match<Expected, Comparison, ResultOfMatched>,
		  Expected,
		  Others...>::type {};

template <typename... Args>
using type_match_t = typename type_match<Args...>::type;

template <typename... Args>
using type_switch = type_match<std::true_type, Args...>;

template <typename... Args>
using type_switch_t = typename type_switch<Args...>::type;

////////////////////////////////////////////////////////////////////////

template <typename type_match_result>
struct _enable_matched : std::enable_if<
							 type_match_result::is_matched_v,
							 typename type_match_result::type> {};

template <typename... Args>
struct enable_matched : _enable_matched<type_match<Args...>> {};

template <typename... Args>
using enable_matched_t = typename enable_matched<Args...>::type;

template <typename... Args>
using enable_switched = enable_matched<std::true_type, Args...>;

template <typename... Args>
using enable_switched_t = typename enable_switched<Args...>::type;

////////////////////////////////////////////////////////////////////////

template <typename T>
struct size_of {
	static constexpr size_t value = sizeof(T);
};

template <>
struct size_of<void> {
	static constexpr size_t value = 0;
};

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T>
inline constexpr auto size_of_v = size_of<T>::value;
#endif

////////////////////////////////////////////////////////////////////////

struct type_info_t;

struct type_id_t {
	type_info_t &(*info)();
};

RUA_FORCE_INLINE bool operator==(type_id_t a, type_id_t b) {
	return a.info == b.info;
}

RUA_FORCE_INLINE bool operator!=(type_id_t a, type_id_t b) {
	return a.info != b.info;
}

struct type_info_t {
	const type_id_t id;

	std::string &(*const desc)();

	void (*const del)(void *);

	void (*const dtor)(void *);

	void (*const dtor_from_any_word)(any_word);

	void (*const copy_ctor)(void *ptr, void *src);

	void copy(void *dest, void *src) {
		if (dtor) {
			dtor(dest);
		}
		copy_ctor(dest, src);
	}

	any_word (*const copy_from_any_word)(const any_word src);

	void (*const move_ctor)(void *ptr, void *src);

	void move(void *dest, void *src) {
		if (dtor) {
			dtor(dest);
		}
		move_ctor(dest, src);
	}

#ifdef RUA_USING_RTTI
	const std::type_info &(*const std_id)();
#endif
};

RUA_FORCE_INLINE bool operator==(type_id_t a, const type_info_t &b) {
	return a.info == b.id.info;
}

RUA_FORCE_INLINE bool operator!=(type_id_t a, const type_info_t &b) {
	return a.info != b.id.info;
}

RUA_FORCE_INLINE bool operator==(const type_info_t &a, type_id_t b) {
	return a.id.info == b.info;
}

RUA_FORCE_INLINE bool operator!=(const type_info_t &a, type_id_t b) {
	return a.id.info != b.info;
}

#ifdef RUA_USING_RTTI

RUA_FORCE_INLINE bool operator==(type_id_t a, std::type_index b) {
	return std::type_index(a.info().std_id()) == b;
}

RUA_FORCE_INLINE bool operator!=(type_id_t a, std::type_index b) {
	return std::type_index(a.info().std_id()) != b;
}

RUA_FORCE_INLINE bool operator==(std::type_index a, type_id_t b) {
	return a == std::type_index(b.info().std_id());
}

RUA_FORCE_INLINE bool operator!=(std::type_index a, type_id_t b) {
	return a != std::type_index(b.info().std_id());
}

RUA_FORCE_INLINE bool operator==(const type_info_t &a, std::type_index b) {
	return std::type_index(a.std_id()) == b;
}

RUA_FORCE_INLINE bool operator!=(const type_info_t &a, std::type_index b) {
	return std::type_index(a.std_id()) != b;
}

RUA_FORCE_INLINE bool operator==(std::type_index a, const type_info_t &b) {
	return a == std::type_index(b.std_id());
}

RUA_FORCE_INLINE bool operator!=(std::type_index a, const type_info_t &b) {
	return a != std::type_index(b.std_id());
}

#endif

template <typename T>
inline typename std::enable_if<std::is_void<T>::value, void (*)(void *)>::type
_type_del() {
	return nullptr;
}

template <typename T>
inline typename std::enable_if<!std::is_void<T>::value, void (*)(void *)>::type
_type_del() {
	return [](void *ptr) { delete reinterpret_cast<T *>(ptr); };
}

template <typename T>
inline typename std::enable_if<
	std::is_void<T>::value || std::is_trivial<T>::value,
	void (*)(void *)>::type
_type_dtor() {
	return nullptr;
}

template <typename T>
inline typename std::enable_if<
	!std::is_void<T>::value && !std::is_trivial<T>::value,
	void (*)(void *)>::type
_type_dtor() {
	return [](void *ptr) { reinterpret_cast<T *>(ptr)->~T(); };
}

template <typename T>
inline typename std::enable_if<
	!std::is_void<T>::value && !std::is_integral<T>::value &&
		!std::is_pointer<T>::value &&
		!(size_of<T>::value <= sizeof(uintptr_t) && std::is_trivial<T>::value),
	void (*)(any_word)>::type
_type_dtor_from_any_word() {
	return [](any_word w) { w.destruct<T>(); };
}

template <typename T>
inline typename std::enable_if<
	std::is_void<T>::value || std::is_integral<T>::value ||
		std::is_pointer<T>::value ||
		(size_of<T>::value <= sizeof(uintptr_t) && std::is_trivial<T>::value),
	void (*)(any_word)>::type
_type_dtor_from_any_word() {
	return nullptr;
}

template <typename T>
inline typename std::enable_if<
	std::is_copy_constructible<T>::value,
	void (*)(void *, void *)>::type
_type_copy_ctor() {
	return [](void *ptr, void *src) {
		new (reinterpret_cast<T *>(ptr)) T(*reinterpret_cast<const T *>(src));
	};
}

template <typename T>
inline typename std::enable_if<
	!std::is_copy_constructible<T>::value,
	void (*)(void *, void *)>::type
_type_copy_ctor() {
	return nullptr;
}

template <typename T>
inline typename std::enable_if<
	std::is_copy_constructible<T>::value,
	any_word (*)(const any_word)>::type
_type_copy_from_any_word() {
	return [](const any_word src) -> any_word { return any_word(src.as<T>()); };
}

template <typename T>
inline typename std::enable_if<
	!std::is_copy_constructible<T>::value,
	any_word (*)(const any_word)>::type
_type_copy_from_any_word() {
	return nullptr;
}

template <typename T>
inline typename std::enable_if<
	std::is_move_constructible<T>::value,
	void (*)(void *, void *)>::type
_type_move_ctor() {
	return [](void *ptr, void *src) {
		new (reinterpret_cast<T *>(ptr))
			T(std::move(*reinterpret_cast<T *>(src)));
	};
}

template <typename T>
inline typename std::enable_if<
	!std::is_move_constructible<T>::value,
	void (*)(void *, void *)>::type
_type_move_ctor() {
	return nullptr;
}

template <typename T>
inline type_info_t &type_info() {
	static type_info_t inf{type_id_t{&type_info<T>},
						   []() -> std::string & {
							   static std::string desc;
							   return desc;
						   },
						   _type_del<T>(),
						   _type_dtor<T>(),
						   _type_dtor_from_any_word<T>(),
						   _type_copy_ctor<T>(),
						   _type_copy_from_any_word<T>(),
						   _type_move_ctor<T>()
#ifdef RUA_USING_RTTI
							   ,
						   []() -> const std::type_info & { return typeid(T); }
#endif
	};
	return inf;
}

template <typename T>
inline constexpr type_id_t type_id() {
	return type_id_t{&type_info<T>};
}

////////////////////////////////////////////////////////////////////////

#define RUA_CONTAINER_OF(member_ptr, type, member)                             \
	reinterpret_cast<type *>(                                                  \
		reinterpret_cast<uintptr_t>(member_ptr) - offsetof(type, member))

#define RUA_IS_BASE_OF_CONCEPT(_B, _D)                                         \
	typename _D, typename = typename std::enable_if<std::is_base_of<           \
					 _B,                                                       \
					 typename std::remove_reference<_D>::type>::value>::type

#define RUA_DERIVED_CONCEPT(_B, _D)                                            \
	typename _D,                                                               \
		typename = typename std::enable_if <                                   \
					   std::is_base_of<                                        \
						   _B,                                                 \
						   typename std::remove_reference<_D>::type>::value && \
				   !std::is_same<_B, _D>::value > ::type

////////////////////////////////////////////////////////////////////////

template <bool IsCopyable, bool IsMovable>
class enable_copy_move;

template <>
class enable_copy_move<true, true> {
public:
	constexpr enable_copy_move() = default;

	enable_copy_move(const enable_copy_move &) = default;
	enable_copy_move(enable_copy_move &&) = default;

	RUA_OVERLOAD_ASSIGNMENT(enable_copy_move)
};

template <>
class enable_copy_move<true, false> {
public:
	constexpr enable_copy_move() = default;

	enable_copy_move(const enable_copy_move &) = default;

	RUA_OVERLOAD_ASSIGNMENT_L(enable_copy_move)
};

template <>
class enable_copy_move<false, true> {
public:
	constexpr enable_copy_move() = default;

	enable_copy_move(enable_copy_move &&) = default;

	RUA_OVERLOAD_ASSIGNMENT_R(enable_copy_move)
};

template <>
class enable_copy_move<false, false> {
public:
	constexpr enable_copy_move() = default;

	enable_copy_move(const enable_copy_move &) = delete;
	enable_copy_move(enable_copy_move &&) = delete;

	enable_copy_move &operator=(const enable_copy_move &) = delete;
	enable_copy_move &operator=(enable_copy_move &&) = delete;
};

template <typename T>
using enable_copy_move_from = enable_copy_move<
	std::is_copy_constructible<T>::value,
	std::is_move_constructible<T>::value>;

////////////////////////////////////////////////////////////////////////

template <typename T>
struct type_identity {
	using type = T;
};

template <typename T>
using type_identity_t = typename type_identity<T>::type;

////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////

template <typename FirstArg, typename... Args>
struct argments_front {
	using type = FirstArg;
};

template <typename... Args>
using argments_front_t = typename argments_front<Args...>::type;

} // namespace rua

#endif
