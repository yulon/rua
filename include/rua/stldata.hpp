#ifndef _RUA_STLDATA_HPP
#define _RUA_STLDATA_HPP

#include <string>
#include <type_traits>

namespace rua {

template <typename T>
inline auto stldata(T &target) -> decltype(target.data()) {
	return target.data();
}

template <typename T>
inline auto stldata(const T &target) -> decltype(target.data()) {
	return target.data();
}

template <typename CharT, typename Traits, typename Allocator>
inline CharT *stldata(std::basic_string<CharT, Traits, Allocator> &target) {
#if RUA_CPP >= RUA_CPP_17
	return target.data();
#else
	return const_cast<CharT *>(target.data());
#endif
}

} // namespace rua

#endif
