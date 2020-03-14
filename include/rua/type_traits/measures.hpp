#ifndef _RUA_TYPE_TRAITS_MEASUREMENTS_HPP
#define _RUA_TYPE_TRAITS_MEASUREMENTS_HPP

#include "../macros.hpp"

#include <cstddef>

namespace rua {

template <typename T>
struct size_of {
	static constexpr size_t value = sizeof(T);
};

template <>
struct size_of<void> {
	static constexpr size_t value = 0;
};

template <typename R, typename... Args>
struct size_of<R(Args...)> {
	static constexpr size_t value = 0;
};

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T>
inline constexpr auto size_of_v = size_of<T>::value;
#endif

////////////////////////////////////////////////////////////////////////////

template <typename T>
struct align_of {
	static constexpr size_t value = alignof(T);
};

template <>
struct align_of<void> {
	static constexpr size_t value = 0;
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

RUA_MULTIDEF_VAR constexpr size_t nullindex = static_cast<size_t>(-1);

template <size_t TypeCount, typename T, typename... Types>
struct _index_of;

template <size_t TypeCount, typename T>
struct _index_of<TypeCount, T> {
	static constexpr size_t value = nullindex;
};

template <size_t TypeCount, typename T, typename Cur, typename... Others>
struct _index_of<TypeCount, T, Cur, Others...> {
	static constexpr size_t value =
		std::is_same<T, Cur>::value ? TypeCount - sizeof...(Others) - 1
									: _index_of<TypeCount, T, Others...>::value;
};

template <typename T, typename... Types>
struct index_of : _index_of<sizeof...(Types), T, Types...> {};

#if RUA_CPP > RUA_CPP_17 || defined(__cpp_inline_variables)
template <typename T, typename... Types>
inline constexpr auto index_of_v = index_of<T, Types...>::value;
#endif

} // namespace rua

#endif
