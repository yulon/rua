#ifndef _rua_util_type_traits_hpp
#define _rua_util_type_traits_hpp

#include "base.hpp"

#include <functional>
#include <type_traits>

namespace rua {

////////////////////////// Standard enhancement ////////////////////////////

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

template <bool Cond, typename T, typename... Args>
using rebind_if_t = typename rebind_if<Cond, T, Args...>::type;

template <bool Cond, typename... T>
struct enable_if;

template <>
struct enable_if<true> {
	using type = void;
};

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

template <bool Cond, typename... T>
using enable_if_t = typename enable_if<Cond, T...>::type;

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

template <bool Cond, typename T, typename... F>
using conditional_t = typename conditional<Cond, T, F...>::type;

///////////////////////////// Standard alias ///////////////////////////////

#ifdef __cpp_lib_bool_constant

template <bool Cond>
using bool_constant = std::bool_constant<Cond>;

#else

template <bool Cond>
using bool_constant = std::integral_constant<bool, Cond>;

#endif

template <typename T>
using remove_cv_t = typename std::remove_cv<T>::type;

template <typename T>
using remove_const_t = typename std::remove_const<T>::type;

template <typename T>
using remove_volatile_t = typename std::remove_volatile<T>::type;

template <typename T>
using add_cv_t = typename std::add_cv<T>::type;

template <typename T>
using add_const_t = typename std::add_const<T>::type;

template <typename T>
using add_volatile_t = typename std::add_volatile<T>::type;

template <typename T>
using remove_reference_t = typename std::remove_reference<T>::type;

template <typename T>
using add_lvalue_reference_t = typename std::add_lvalue_reference<T>::type;

template <typename T>
using add_rvalue_reference_t = typename std::add_rvalue_reference<T>::type;

template <typename T>
using remove_pointer_t = typename std::remove_pointer<T>::type;

template <typename T>
using add_pointer_t = typename std::add_pointer<T>::type;

template <typename T>
using make_signed_t = typename std::make_signed<T>::type;

template <typename T>
using make_unsigned_t = typename std::make_unsigned<T>::type;

template <typename T>
using remove_extent_t = typename std::remove_extent<T>::type;

template <typename T>
using remove_all_extents_t = typename std::remove_all_extents<T>::type;

template <typename T>
using decay_t = typename std::decay<T>::type;

template <typename... T>
using common_type_t = typename std::common_type<T...>::type;

template <typename T>
using underlying_type_t = typename std::underlying_type<T>::type;

///////////////////////// Standard compatibility ///////////////////////////

template <typename...>
using void_t = void;

template <typename T>
struct is_bounded_array : std::false_type {};

template <typename T, size_t N>
struct is_bounded_array<T[N]> : std::true_type {};

template <typename T>
struct is_unbounded_array : std::false_type {};

template <typename T>
struct is_unbounded_array<T[]> : std::true_type {};

#ifdef __cpp_lib_is_null_pointer

template <typename T>
using is_null_pointer = std::is_null_pointer<T>;

#else

template <typename T>
struct is_null_pointer : std::is_same<remove_cv_t<T>, std::nullptr_t> {};

#endif

#ifdef __cpp_lib_type_identity

template <typename T>
using type_identity = std::type_identity<T>;

#else

template <typename T>
struct type_identity {
	using type = T;
};

#endif

template <typename T>
using type_identity_t = typename type_identity<T>::type;

#ifdef __cpp_lib_remove_cvref

template <typename T>
using remove_cvref = std::remove_cvref<T>;

#else

template <typename T>
struct remove_cvref : std::remove_cv<remove_reference_t<T>> {};

#endif

template <typename T>
using remove_cvref_t = typename remove_cvref<T>::type;

template <typename...>
struct conjunction : std::true_type {};

template <typename B1>
struct conjunction<B1> : B1 {};

template <typename B1, typename... Bn>
struct conjunction<B1, Bn...>
	: conditional_t<static_cast<bool>(B1::value), conjunction<Bn...>, B1> {};

template <typename...>
struct disjunction : std::false_type {};

template <typename B1>
struct disjunction<B1> : B1 {};

template <typename B1, typename... Bn>
struct disjunction<B1, Bn...>
	: conditional_t<static_cast<bool>(B1::value), B1, disjunction<Bn...>> {};

#ifdef __cpp_lib_unwrap_ref

template <typename T>
using unwrap_reference = typename std::unwrap_reference<T>;

template <typename T>
using unwrap_ref_decay = typename std::unwrap_ref_decay<T>;

#else

template <typename T>
struct unwrap_reference {
	using type = T;
};

template <typename U>
struct unwrap_reference<std::reference_wrapper<U>> {
	using type = U &;
};

template <typename T>
struct unwrap_ref_decay : unwrap_reference<decay_t<T>> {};

#endif

template <typename T>
using unwrap_reference_t = typename unwrap_reference<T>::type;

template <typename T>
using unwrap_ref_decay_t = typename unwrap_ref_decay<T>::type;

////////////////////////////// Non-standard ////////////////////////////////

template <typename T>
struct is_reference_wrapper : std::false_type {};
template <typename U>
struct is_reference_wrapper<std::reference_wrapper<U>> : std::true_type {};

////////////////////////////////////////////////////////////////////////////

template <typename T, typename = void>
struct size_of {
	static constexpr size_t value = 0;
};

template <>
struct size_of<void> {
	static constexpr size_t value = 0;
};

template <typename T>
struct size_of<
	T,
	enable_if_t<!std::is_function<T>::value && !std::is_reference<T>::value>> {
	static constexpr size_t value = sizeof(T);
};

#ifdef RUA_HAS_INLINE_VAR
template <typename T>
inline constexpr auto size_of_v = size_of<T>::value;
#endif

////////////////////////////////////////////////////////////////////////////

template <typename T, typename = void>
struct align_of {
	static constexpr size_t value = 0;
};

template <>
struct align_of<void> {
	static constexpr size_t value = 0;
};

template <typename T>
struct align_of<
	T,
	enable_if_t<!std::is_function<T>::value && !std::is_reference<T>::value>> {
	static constexpr size_t value = alignof(T);
};

#ifdef RUA_HAS_INLINE_VAR
template <typename T>
inline constexpr auto align_of_v = align_of<T>::value;
#endif

////////////////////////////////////////////////////////////////////////////

template <size_t Size>
struct align_of_size {
	static constexpr size_t value =
		sizeof(void *) % Size ? sizeof(void *) / (sizeof(void *) / Size) : Size;
};

#ifdef RUA_HAS_INLINE_VAR
template <size_t Size>
inline constexpr auto align_of_size_v = align_of_size<Size>::value;
#endif

////////////////////////////////////////////////////////////////////////////

template <typename... Types>
struct max_size_of;

template <typename Last>
struct max_size_of<Last> {
	static constexpr size_t value = size_of<Last>::value;
};

template <typename First, typename... Others>
struct max_size_of<First, Others...> {
	static constexpr size_t value =
		size_of<First>::value > max_size_of<Others...>::value
			? size_of<First>::value
			: max_size_of<Others...>::value;
};

#ifdef RUA_HAS_INLINE_VAR
template <typename... Types>
inline constexpr auto max_size_of_v = max_size_of<Types...>::value;
#endif

////////////////////////////////////////////////////////////////////////////

template <typename... Types>
struct max_align_of;

template <typename Last>
struct max_align_of<Last> {
	static constexpr size_t value = align_of<Last>::value;
};

template <typename First, typename... Others>
struct max_align_of<First, Others...> {
	static constexpr size_t value =
		align_of<First>::value > max_align_of<Others...>::value
			? align_of<First>::value
			: max_align_of<Others...>::value;
};

#ifdef RUA_HAS_INLINE_VAR
template <typename... Types>
inline constexpr auto max_align_of_v = max_align_of<Types...>::value;
#endif

////////////////////////////////////////////////////////////////////////////

template <typename FirstArg, typename... Args>
struct front {
	using type = FirstArg;
};

template <typename... Args>
using front_t = typename front<Args...>::type;

////////////////////////////////////////////////////////////////////////////

template <size_t C, typename T, typename... Types>
struct _index_of;

template <size_t C, typename T>
struct _index_of<C, T> {
	static constexpr size_t value = static_cast<size_t>(-1);
};

template <size_t C, typename T, typename Cur, typename... Others>
struct _index_of<C, T, Cur, Others...> {
	static constexpr size_t value = std::is_same<T, Cur>::value
										? C - sizeof...(Others) - 1
										: _index_of<C, T, Others...>::value;
};

template <typename T, typename... Types>
struct index_of : _index_of<sizeof...(Types), T, Types...> {};

#ifdef RUA_HAS_INLINE_VAR
template <typename T, typename... Types>
inline constexpr auto index_of_v = index_of<T, Types...>::value;
#endif

////////////////////////////////////////////////////////////////////////////

template <typename T, typename... Types>
struct _same;

template <typename T>
struct _same<T> {
	using type = not_matched_t;
};

template <typename T, typename Cur, typename... Others>
struct _same<T, Cur, Others...> : std::conditional<
									  std::is_same<T, decay_t<Cur>>::value,
									  Cur,
									  typename _same<T, Others...>::type> {};

template <typename T, typename... Types>
struct _convertible;

template <typename T>
struct _convertible<T> {
	using type = not_matched_t;
};

template <typename T, typename Cur, typename... Others>
struct _convertible<T, Cur, Others...>
	: std::conditional<
		  std::is_convertible<T, Cur>::value,
		  Cur,
		  typename _convertible<T, Others...>::type> {};

template <typename Samed, typename Convertible>
struct _same_or_convertible {
	using type = Samed;
};

template <typename Convertible>
struct _same_or_convertible<not_matched_t, Convertible> {
	using type = Convertible;
};

template <>
struct _same_or_convertible<not_matched_t, not_matched_t> {};

template <typename T, typename... Types>
struct convertible : _same_or_convertible<
						 typename _same<decay_t<T>, Types...>::type,
						 typename _convertible<T, Types...>::type> {};

template <typename T, typename... Args>
using convertible_t = typename convertible<T, Args...>::type;

////////////////////////////////////////////////////////////////////////////

template <typename From, typename To>
struct is_same_or_convertible
	: disjunction<std::is_same<From, To>, std::is_convertible<From, To>> {};

template <typename From, typename... To>
struct or_convertible : disjunction<is_same_or_convertible<From, To>...> {};

} // namespace rua

#endif
