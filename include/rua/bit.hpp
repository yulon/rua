#ifndef _RUA_BIT_HPP
#define _RUA_BIT_HPP

#include "generic_ptr.hpp"
#include "macros.hpp"
#include "type_traits/measures.hpp"
#include "type_traits/std_patch.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace rua {

// bit_get from generic_ptr

template <typename To, typename NCTo = remove_cv_t<To>>
RUA_FORCE_INLINE enable_if_t<
	std::is_trivial<To>::value && !std::is_same<NCTo, char>::value &&
		!std::is_same<NCTo, unsigned char>::value,
	To>
bit_get(generic_ptr ptr) {
	alignas(alignof(To)) char sto[sizeof(To)];
	memcpy(&sto, ptr, sizeof(To));
	return reinterpret_cast<To &>(sto);
}

template <typename To, typename NCTo = remove_cv_t<To>>
RUA_FORCE_INLINE enable_if_t<
	std::is_trivial<To>::value && (std::is_same<NCTo, char>::value ||
								   std::is_same<NCTo, unsigned char>::value),
	const NCTo &>
bit_get(generic_ptr ptr) {
	return *ptr.as<To *>();
}

template <typename To, typename NCTo = remove_cv_t<To>>
RUA_FORCE_INLINE enable_if_t<
	std::is_trivial<To>::value,
	conditional_t<
		!std::is_same<NCTo, char>::value &&
			!std::is_same<NCTo, unsigned char>::value,
		To,
		const NCTo &>>
bit_get(generic_ptr ptr, ptrdiff_t offset) {
	return bit_get<To>(ptr + offset);
}

template <typename To, typename NCTo = remove_cv_t<To>>
RUA_FORCE_INLINE enable_if_t<
	std::is_trivial<To>::value,
	conditional_t<
		!std::is_same<NCTo, char>::value &&
			!std::is_same<NCTo, unsigned char>::value,
		To,
		const NCTo &>>
bit_get_aligned(generic_ptr ptr, ptrdiff_t ix) {
	return bit_get<To>(ptr + ix * sizeof(To));
}

// bit_get from From *

template <typename To, typename From>
RUA_FORCE_INLINE enable_if_t<
	std::is_trivial<To>::value && (size_of<To>::value > sizeof(char) ||
								   size_of<To>::value > alignof(char)),
	To>
bit_get(From *ptr) {
	alignas(alignof(To)) char sto[sizeof(To)];
	memcpy(&sto, reinterpret_cast<const void *>(ptr), sizeof(To));
	return reinterpret_cast<To &>(sto);
}

template <typename To, typename From, typename NCFrom = remove_cv_t<From>>
RUA_FORCE_INLINE enable_if_t<
	std::is_trivial<To>::value && size_of<To>::value == sizeof(char) &&
		align_of<To>::value == alignof(char) &&
		(std::is_same<NCFrom, char>::value ||
		 std::is_same<NCFrom, unsigned char>::value),
	const To &>
bit_get(From *ptr) {
	return *reinterpret_cast<const To *>(ptr);
}

template <
	typename To,
	typename From,
	typename NCTo = remove_cv_t<To>,
	typename NCFrom = remove_cv_t<From>>
RUA_FORCE_INLINE enable_if_t<
	std::is_trivial<To>::value &&
		(std::is_same<NCTo, char>::value ||
		 std::is_same<NCTo, unsigned char>::value) &&
		!std::is_same<NCFrom, char>::value &&
		!std::is_same<NCFrom, unsigned char>::value,
	const NCTo &>
bit_get(From *ptr) {
	return *reinterpret_cast<const NCTo *>(ptr);
}

template <typename To, typename From>
RUA_FORCE_INLINE decltype(bit_get<To>(std::declval<From *>()))
bit_get(From *ptr, ptrdiff_t offset) {
	return bit_get<To>(
		reinterpret_cast<From *>(reinterpret_cast<uintptr_t>(ptr) + offset));
}

template <typename To, typename From>
RUA_FORCE_INLINE decltype(bit_get<To>(std::declval<From *>()))
bit_get_aligned(From *ptr, ptrdiff_t ix) {
	return bit_get<To>(ptr, ix * sizeof(To));
}

// bit_set to generic_ptr

template <typename From>
RUA_FORCE_INLINE enable_if_t<std::is_trivial<From>::value>
bit_set(generic_ptr ptr, const From &val) {
	memcpy(ptr, &val, sizeof(From));
}

template <typename From>
RUA_FORCE_INLINE enable_if_t<std::is_trivial<From>::value>
bit_set(generic_ptr ptr, ptrdiff_t offset, const From &val) {
	bit_set<From>(ptr + offset, val);
}

template <typename From>
RUA_FORCE_INLINE enable_if_t<std::is_trivial<From>::value>
bit_set_aligned(generic_ptr ptr, ptrdiff_t ix, const From &val) {
	bit_set<From>(ptr + ix * sizeof(From), val);
}

// bit_set to To *

template <typename From, typename To>
RUA_FORCE_INLINE enable_if_t<std::is_trivial<From>::value>
bit_set(To *ptr, const From &val) {
	memcpy(reinterpret_cast<void *>(ptr), &val, sizeof(From));
}

template <typename From, typename To>
RUA_FORCE_INLINE enable_if_t<std::is_trivial<From>::value>
bit_set(To *ptr, ptrdiff_t offset, const From &val) {
	bit_set<From>(
		reinterpret_cast<To *>(reinterpret_cast<uintptr_t>(ptr) + offset), val);
}

template <typename From, typename To>
RUA_FORCE_INLINE enable_if_t<std::is_trivial<From>::value>
bit_set_aligned(To *ptr, ptrdiff_t ix, const From &val) {
	bit_set<From>(ptr, ix * sizeof(From), val);
}

// bit_cast

template <typename To, typename From>
RUA_FORCE_INLINE enable_if_t<
	(sizeof(To) == sizeof(From)) && std::is_trivially_copyable<From>::value &&
		std::is_trivial<To>::value,
	decltype(bit_get<To>(std::declval<const From *>()))>
bit_cast(const From &src) {
	return bit_get<To>(&src);
}

} // namespace rua

#endif
