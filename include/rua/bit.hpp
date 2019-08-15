#ifndef _RUA_BIT_HPP
#define _RUA_BIT_HPP

#include "macros.hpp"

#include <cstdint>
#include <cstring>
#include <type_traits>

namespace rua {

template <typename T, typename P>
RUA_FORCE_INLINE typename std::enable_if<std::is_trivial<T>::value, T>::type
bit_get(P *ptr) {
	typename std::aligned_storage<sizeof(T), alignof(T)>::type sto;
	memcpy(&sto, reinterpret_cast<const void *>(ptr), sizeof(T));
	return reinterpret_cast<T &>(sto);
}

template <typename T, typename P>
RUA_FORCE_INLINE typename std::enable_if<std::is_trivial<T>::value, T>::type
bit_get(P *ptr, ptrdiff_t offset) {
	return bit_get<T>(reinterpret_cast<const uint8_t *>(ptr) + offset);
}

template <typename T, typename P>
RUA_FORCE_INLINE typename std::enable_if<std::is_trivial<T>::value, T>::type
bit_get_aligned(P *ptr, ptrdiff_t ix) {
	return bit_get<T>(reinterpret_cast<const T *>(ptr) + ix);
}

template <typename T, typename P>
RUA_FORCE_INLINE typename std::enable_if<std::is_trivial<T>::value>::type
bit_set(P *ptr, const T &val) {
	memcpy(reinterpret_cast<void *>(ptr), &val, sizeof(T));
}

template <typename T, typename P>
RUA_FORCE_INLINE typename std::enable_if<std::is_trivial<T>::value>::type
bit_set(P *ptr, ptrdiff_t offset, const T &val) {
	bit_set<T>(reinterpret_cast<uint8_t *>(ptr) + offset, val);
}

template <typename T, typename P>
RUA_FORCE_INLINE typename std::enable_if<std::is_trivial<T>::value>::type
bit_set_aligned(P *ptr, ptrdiff_t ix, const T &val) {
	return bit_set<T>(reinterpret_cast<T *>(ptr) + ix, val);
}

template <typename To, typename From>
RUA_FORCE_INLINE typename std::enable_if<
	(sizeof(To) == sizeof(From)) && std::is_trivially_copyable<From>::value &&
		std::is_trivial<To>::value,
	To>::type
bit_cast(const From &src) {
	return bit_get<To>(&src);
}

} // namespace rua

#endif
