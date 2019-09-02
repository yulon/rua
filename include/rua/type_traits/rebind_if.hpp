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

} // namespace rua

#endif
