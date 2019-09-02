#ifndef _RUA_TYPE_TRAITS_ARGMENTS_HPP
#define _RUA_TYPE_TRAITS_ARGMENTS_HPP

namespace rua {

template <typename FirstArg, typename... Args>
struct argments_front {
	using type = FirstArg;
};

template <typename... Args>
using argments_front_t = typename argments_front<Args...>::type;

} // namespace rua

#endif
