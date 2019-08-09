#ifndef _RUA_BIT_HPP
#define _RUA_BIT_HPP

#include "macros.hpp"

#include <cstdint>
#include <cstring>
#include <type_traits>

namespace rua {

template <typename T>
RUA_FORCE_INLINE typename std::enable_if<std::is_trivial<T>::value, T>::type
bit_get(const void *ptr, ptrdiff_t offset = 0) {
	typename std::aligned_storage<sizeof(T), alignof(T)>::type sto;
	memcpy(
		&sto,
		reinterpret_cast<const void *>(
			reinterpret_cast<uintptr_t>(ptr) + offset),
		sizeof(T));
	return reinterpret_cast<T &>(sto);
}

template <typename T>
RUA_FORCE_INLINE typename std::enable_if<std::is_trivial<T>::value, T>::type
bit_aligned_get(const void *ptr, ptrdiff_t ix = 0) {
	return bit_get<T>(ptr, ix * sizeof(T));
}

template <
	typename T,
	typename = typename std::enable_if<std::is_trivial<T>::value, T>::type>
RUA_FORCE_INLINE void bit_set(void *ptr, const T &val) {
	memcpy(ptr, &val, sizeof(T));
}

template <
	typename T,
	typename = typename std::enable_if<std::is_trivial<T>::value, T>::type>
RUA_FORCE_INLINE void bit_set(void *ptr, ptrdiff_t offset, const T &val) {
	bit_set<T>(
		reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(ptr) + offset),
		val);
}

template <
	typename T,
	typename = typename std::enable_if<std::is_trivial<T>::value, T>::type>
RUA_FORCE_INLINE void bit_aligned_set(void *ptr, ptrdiff_t ix, const T &val) {
	return bit_set<T>(ptr, ix * sizeof(T), val);
}

template <class To, class From>
RUA_FORCE_INLINE typename std::enable_if<
	(sizeof(To) == sizeof(From)) && std::is_trivially_copyable<From>::value &&
		std::is_trivial<To>::value,
	To>::type
bit_cast(const From &src) noexcept {
	return bit_get(&src);
}

} // namespace rua

#endif
