#ifndef _RUA_TYPES_TRAITS_HPP
#define _RUA_TYPES_TRAITS_HPP

#include "util.hpp"

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

template <std::size_t Len, std::size_t Align = Len>
using aligned_storage_t = typename std::aligned_storage<Len, Align>::type;

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
	: conditional_t<bool(B1::value), conjunction<Bn...>, B1> {};

#ifdef __cpp_lib_is_invocable

template <typename F, typename... Args>
using invoke_result = std::invoke_result<F, Args...>;

template <typename F, typename... Args>
using invoke_result_t = std::invoke_result_t<F, Args...>;

#else

template <typename F, typename... Args>
using invoke_result_t = decltype(std::declval<F>()(std::declval<Args>()...));

template <typename F, typename... Args>
struct invoke_result {
	using type = invoke_result_t<F, Args...>;
};

#endif

#ifdef __cpp_lib_is_invocable

template <typename F, typename... Args>
using is_invocable = std::is_invocable<F, Args...>;

#else

template <typename...>
struct _is_invocable : std::false_type {};

template <typename F, typename... Args>
struct _is_invocable<void_t<invoke_result_t<F, Args...>>, F, Args...>
	: std::true_type {};

template <typename F, typename... Args>
struct is_invocable : _is_invocable<void, F, Args...> {};

#endif

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename F, typename... Args>
inline constexpr auto is_invocable_v = is_invocable<F, Args...>::value;
#endif

////////////////////////////// Non-standard ////////////////////////////////

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

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
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

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T>
inline constexpr auto align_of_v = align_of<T>::value;
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

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
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

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
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

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T, typename... Types>
inline constexpr auto index_of_v = index_of<T, Types...>::value;
#endif

////////////////////////////////////////////////////////////////////////////

template <typename T>
struct remove_member_pointer {
	using type = T;
};

template <typename T, typename E>
struct remove_member_pointer<E T::*> {
	using type = E;
};

template <typename T, typename E>
struct remove_member_pointer<E T::*const> {
	using type = E;
};

template <typename T, typename E>
struct remove_member_pointer<E T::*volatile> {
	using type = E;
};

template <typename T, typename E>
struct remove_member_pointer<E T::*const volatile> {
	using type = E;
};

template <typename T>
using remove_member_pointer_t = typename remove_member_pointer<T>::type;

////////////////////////////////////////////////////////////////////////////

template <typename T>
struct remove_function_noexcept {
	using type = T;
};

template <typename Ret, typename... Args>
struct remove_function_noexcept<Ret(Args...) noexcept> {
	using type = Ret(Args...);
};

template <typename Ret, typename... Args>
struct remove_function_noexcept<Ret(Args..., ...) noexcept> {
	using type = Ret(Args..., ...);
};

template <typename Ret, typename... Args>
struct remove_function_noexcept<Ret(Args...) const noexcept> {
	using type = Ret(Args...) const;
};

template <typename Ret, typename... Args>
struct remove_function_noexcept<Ret(Args...) volatile noexcept> {
	using type = Ret(Args...) volatile;
};

template <typename Ret, typename... Args>
struct remove_function_noexcept<Ret(Args...) const volatile noexcept> {
	using type = Ret(Args...) const volatile;
};

template <typename Ret, typename... Args>
struct remove_function_noexcept<Ret(Args..., ...) const noexcept> {
	using type = Ret(Args...) const;
};

template <typename Ret, typename... Args>
struct remove_function_noexcept<Ret(Args..., ...) volatile noexcept> {
	using type = Ret(Args...) volatile;
};

template <typename Ret, typename... Args>
struct remove_function_noexcept<Ret(Args..., ...) const volatile noexcept> {
	using type = Ret(Args...) const volatile;
};

template <typename Ret, typename... Args>
struct remove_function_noexcept<Ret(Args...) &noexcept> {
	using type = Ret(Args...) &;
};

template <typename Ret, typename... Args>
struct remove_function_noexcept<Ret(Args...) const &noexcept> {
	using type = Ret(Args...) const &;
};

template <typename Ret, typename... Args>
struct remove_function_noexcept<Ret(Args...) volatile &noexcept> {
	using type = Ret(Args...) volatile &;
};

template <typename Ret, typename... Args>
struct remove_function_noexcept<Ret(Args...) const volatile &noexcept> {
	using type = Ret(Args...) const volatile &;
};

template <typename Ret, typename... Args>
struct remove_function_noexcept<Ret(Args..., ...) &noexcept> {
	using type = Ret(Args..., ...) &;
};

template <typename Ret, typename... Args>
struct remove_function_noexcept<Ret(Args..., ...) const &noexcept> {
	using type = Ret(Args..., ...) const &;
};

template <typename Ret, typename... Args>
struct remove_function_noexcept<Ret(Args..., ...) volatile &noexcept> {
	using type = Ret(Args..., ...) volatile &;
};

template <typename Ret, typename... Args>
struct remove_function_noexcept<Ret(Args..., ...) const volatile &noexcept> {
	using type = Ret(Args..., ...) const volatile &;
};

template <typename Ret, typename... Args>
struct remove_function_noexcept<Ret(Args...) &&noexcept> {
	using type = Ret(Args...) &&;
};

template <typename Ret, typename... Args>
struct remove_function_noexcept<Ret(Args...) const &&noexcept> {
	using type = Ret(Args...) const &&;
};

template <typename Ret, typename... Args>
struct remove_function_noexcept<Ret(Args...) volatile &&noexcept> {
	using type = Ret(Args...) volatile &&;
};

template <typename Ret, typename... Args>
struct remove_function_noexcept<Ret(Args...) const volatile &&noexcept> {
	using type = Ret(Args...) const volatile &&;
};

template <typename Ret, typename... Args>
struct remove_function_noexcept<Ret(Args..., ...) &&noexcept> {
	using type = Ret(Args..., ...) &&;
};

template <typename Ret, typename... Args>
struct remove_function_noexcept<Ret(Args..., ...) const &&noexcept> {
	using type = Ret(Args..., ...) const &&;
};

template <typename Ret, typename... Args>
struct remove_function_noexcept<Ret(Args..., ...) volatile &&noexcept> {
	using type = Ret(Args..., ...) volatile &&;
};

template <typename Ret, typename... Args>
struct remove_function_noexcept<Ret(Args..., ...) const volatile &&noexcept> {
	using type = Ret(Args..., ...) const volatile &&;
};

template <typename T>
using remove_function_noexcept_t = typename remove_function_noexcept<T>::type;

////////////////////////////////////////////////////////////////////////////

template <typename T>
struct is_function_has_noexcept
	: bool_constant<!std::is_same<T, remove_function_noexcept_t<T>>::value> {};

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T>
inline constexpr auto is_function_has_noexcept_v =
	is_function_has_noexcept<T>::value;
#endif

////////////////////////////////////////////////////////////////////////////

template <typename T>
struct remove_function_lref {
	using type = T;
};

template <typename Ret, typename... Args>
struct remove_function_lref<Ret(Args...) &> {
	using type = Ret(Args...);
};

template <typename Ret, typename... Args>
struct remove_function_lref<Ret(Args...) const &> {
	using type = Ret(Args...) const;
};

template <typename Ret, typename... Args>
struct remove_function_lref<Ret(Args...) volatile &> {
	using type = Ret(Args...) volatile;
};

template <typename Ret, typename... Args>
struct remove_function_lref<Ret(Args...) const volatile &> {
	using type = Ret(Args...) const volatile;
};

template <typename Ret, typename... Args>
struct remove_function_lref<Ret(Args..., ...) &> {
	using type = Ret(Args..., ...);
};

template <typename T>
using remove_function_lref_t = typename remove_function_lref<T>::type;

////////////////////////////////////////////////////////////////////////////

template <typename T, typename RmNoexcept = remove_function_noexcept_t<T>>
struct is_function_has_lref : bool_constant<!std::is_same<
								  RmNoexcept,
								  remove_function_lref_t<RmNoexcept>>::value> {
};

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T>
inline constexpr auto is_function_has_lref_v = is_function_has_lref<T>::value;
#endif

////////////////////////////////////////////////////////////////////////////

template <typename T>
struct remove_function_rref {
	using type = T;
};

template <typename Ret, typename... Args>
struct remove_function_rref<Ret(Args..., ...) const &> {
	using type = Ret(Args..., ...) const;
};

template <typename Ret, typename... Args>
struct remove_function_rref<Ret(Args..., ...) volatile &> {
	using type = Ret(Args..., ...) volatile;
};

template <typename Ret, typename... Args>
struct remove_function_rref<Ret(Args..., ...) const volatile &> {
	using type = Ret(Args..., ...) const volatile;
};

template <typename Ret, typename... Args>
struct remove_function_rref<Ret(Args...) &&> {
	using type = Ret(Args...);
};

template <typename Ret, typename... Args>
struct remove_function_rref<Ret(Args...) const &&> {
	using type = Ret(Args...) const;
};

template <typename Ret, typename... Args>
struct remove_function_rref<Ret(Args...) volatile &&> {
	using type = Ret(Args...) volatile;
};

template <typename Ret, typename... Args>
struct remove_function_rref<Ret(Args...) const volatile &&> {
	using type = Ret(Args...) const volatile;
};

template <typename Ret, typename... Args>
struct remove_function_rref<Ret(Args..., ...) &&> {
	using type = Ret(Args..., ...);
};

template <typename Ret, typename... Args>
struct remove_function_rref<Ret(Args..., ...) const &&> {
	using type = Ret(Args..., ...) const;
};

template <typename Ret, typename... Args>
struct remove_function_rref<Ret(Args..., ...) volatile &&> {
	using type = Ret(Args..., ...) volatile;
};

template <typename Ret, typename... Args>
struct remove_function_rref<Ret(Args..., ...) const volatile &&> {
	using type = Ret(Args..., ...) const volatile;
};

template <typename T>
using remove_function_rref_t = typename remove_function_rref<T>::type;

////////////////////////////////////////////////////////////////////////////

template <typename T, typename RmNoexcept = remove_function_noexcept_t<T>>
struct is_function_has_rref : bool_constant<!std::is_same<
								  RmNoexcept,
								  remove_function_rref_t<RmNoexcept>>::value> {
};

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T>
inline constexpr auto is_function_has_rref_v = is_function_has_rref<T>::value;
#endif

////////////////////////////////////////////////////////////////////////////

template <typename T>
struct remove_function_ref : remove_function_lref<remove_function_rref_t<T>> {};

template <typename T>
using remove_function_ref_t = typename remove_function_ref<T>::type;

////////////////////////////////////////////////////////////////////////////

template <typename T, typename RmNoexcept = remove_function_noexcept_t<T>>
struct is_function_has_ref
	: bool_constant<
		  !std::is_same<RmNoexcept, remove_function_ref_t<RmNoexcept>>::value> {
};

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T>
inline constexpr auto is_function_has_ref_v = is_function_has_ref<T>::value;
#endif

////////////////////////////////////////////////////////////////////////////

template <typename T>
struct remove_function_volatile {
	using type = T;
};

template <typename Ret, typename... Args>
struct remove_function_volatile<Ret(Args...) volatile> {
	using type = Ret(Args...);
};

template <typename Ret, typename... Args>
struct remove_function_volatile<Ret(Args...) const volatile> {
	using type = Ret(Args...) const;
};

template <typename Ret, typename... Args>
struct remove_function_volatile<Ret(Args..., ...) volatile> {
	using type = Ret(Args..., ...);
};

template <typename Ret, typename... Args>
struct remove_function_volatile<Ret(Args..., ...) const volatile> {
	using type = Ret(Args..., ...) const;
};

template <typename T>
using remove_function_volatile_t = typename remove_function_volatile<T>::type;

////////////////////////////////////////////////////////////////////////////

template <
	typename T,
	typename RmRefNoexcept =
		remove_function_ref_t<remove_function_noexcept_t<T>>>
struct is_function_has_volatile
	: bool_constant<!std::is_same<
		  RmRefNoexcept,
		  remove_function_volatile_t<RmRefNoexcept>>::value> {};

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T>
inline constexpr auto is_function_has_volatile_v =
	is_function_has_volatile<T>::value;
#endif

////////////////////////////////////////////////////////////////////////////

template <typename T>
struct remove_function_const {
	using type = T;
};

template <typename Ret, typename... Args>
struct remove_function_const<Ret(Args...) const> {
	using type = Ret(Args...);
};

template <typename Ret, typename... Args>
struct remove_function_const<Ret(Args..., ...) const> {
	using type = Ret(Args..., ...);
};

template <typename T>
using remove_function_const_t = typename remove_function_const<T>::type;

////////////////////////////////////////////////////////////////////////////

template <
	typename T,
	typename RmVRefNoexcept = remove_function_volatile_t<
		remove_function_ref_t<remove_function_noexcept_t<T>>>>
struct is_function_has_const
	: bool_constant<!std::is_same<
		  RmVRefNoexcept,
		  remove_function_const_t<RmVRefNoexcept>>::value> {};

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T>
inline constexpr auto is_function_has_const_v = is_function_has_const<T>::value;
#endif

////////////////////////////////////////////////////////////////////////////

template <typename T>
struct decay_function {
	using type = remove_function_const_t<remove_function_volatile_t<
		remove_function_ref_t<remove_function_noexcept_t<T>>>>;
};

template <typename T>
using decay_function_t = typename decay_function<T>::type;

////////////////////////////////////////////////////////////////////////////

template <typename T>
struct remove_function_va {
	using type = T;
};

template <typename Ret, typename... Args>
struct remove_function_va<Ret(Args..., ...)> {
	using type = Ret(Args...);
};

template <typename T>
using remove_function_va_t = typename remove_function_va<T>::type;

////////////////////////////////////////////////////////////////////////////

template <typename T, typename DecayFunc = decay_function_t<T>>
struct is_function_has_va
	: bool_constant<
		  !std::is_same<DecayFunc, remove_function_va_t<DecayFunc>>::value> {};

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T>
inline constexpr auto is_function_has_va_v = is_function_has_va<T>::value;
#endif

////////////////////////////////////////////////////////////////////////////

template <typename Callable, typename = void>
struct callable_prototype {};

template <typename Ret, typename... Args>
struct callable_prototype<Ret (*)(Args...)> {
	using type = Ret(Args...);
};

template <typename Callable>
struct callable_prototype<Callable, void_t<decltype(&Callable::operator())>> {
	using type = decay_function_t<
		remove_member_pointer_t<decltype(&Callable::operator())>>;
};

template <typename Callable>
using callable_prototype_t = typename callable_prototype<Callable>::type;

} // namespace rua

#endif
