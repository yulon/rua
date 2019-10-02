#ifndef _RUA_TYPE_TRAITS_REBIND_IF_HPP
#define _RUA_TYPE_TRAITS_REBIND_IF_HPP

namespace rua {

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

} // namespace rua

#endif
