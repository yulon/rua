#ifndef _RUA_BIT_HPP
#define _RUA_BIT_HPP

#include "generic_ptr.hpp"
#include "macros.hpp"
#include "types/traits.hpp"
#include "types/util.hpp"

#include <cstdio>
#include <cstring>

namespace rua {

// bit_as from generic_ptr

template <typename To>
inline To &bit_as(generic_ptr ptr) {
	return *ptr.as<To *>();
}

template <typename To>
inline To &bit_as(generic_ptr ptr, ptrdiff_t offset) {
	return bit_as<To>(ptr + offset);
}

template <typename To>
inline To &bit_aligned_as(generic_ptr ptr, ptrdiff_t ix) {
	return bit_as<To>(ptr, ix * sizeof(To));
}

// bit_as from From *

template <
	typename To,
	typename From,
	typename Result =
		conditional_t<std::is_const<From>::value, add_const_t<To>, To>>
inline Result &bit_as(From *ptr) {
	return *reinterpret_cast<Result *>(ptr);
}

template <
	typename To,
	typename From,
	typename Result =
		conditional_t<std::is_const<From>::value, add_const_t<To>, To>>
inline Result &bit_as(From *ptr, ptrdiff_t offset) {
	return *reinterpret_cast<Result *>(
		reinterpret_cast<From *>(reinterpret_cast<uintptr_t>(ptr) + offset));
}

template <
	typename To,
	typename From,
	typename Result =
		conditional_t<std::is_const<From>::value, add_const_t<To>, To>>
inline Result &bit_aligned_as(From *ptr, ptrdiff_t ix) {
	return *reinterpret_cast<Result *>(reinterpret_cast<From *>(
		reinterpret_cast<uintptr_t>(ptr) + ix * sizeof(To)));
}

// bit_get from generic_ptr

template <typename To>
inline enable_if_t<std::is_trivial<To>::value, To> bit_get(generic_ptr ptr) {
	To val;
	memcpy(&val, ptr, sizeof(To));
	return val;
}

template <typename To>
inline enable_if_t<!std::is_trivially_constructible<To>::value, To>
bit_get(generic_ptr ptr) {
	alignas(alignof(To)) uchar sto[sizeof(To)];
	memcpy(&sto[0], ptr, sizeof(To));
	return *reinterpret_cast<To *>(&sto[0]);
}

template <typename To>
inline decltype(bit_get<To>(nullptr))
bit_get(generic_ptr ptr, ptrdiff_t offset) {
	return bit_get<To>(ptr + offset);
}

template <typename To>
inline decltype(bit_get<To>(nullptr))
bit_aligned_get(generic_ptr ptr, ptrdiff_t ix) {
	return bit_get<To>(ptr + ix * sizeof(To));
}

// bit_get from From *

template <typename To, typename From>
inline enable_if_t<std::is_trivial<To>::value, To> bit_get(From *ptr) {
	To val;
	memcpy(&val, ptr, sizeof(To));
	return val;
}

template <typename To, typename From>
inline enable_if_t<!std::is_trivially_constructible<To>::value, To>
bit_get(From *ptr) {
	alignas(alignof(To)) uchar sto[sizeof(To)];
	memcpy(&sto[0], ptr, sizeof(To));
	return *reinterpret_cast<To *>(&sto[0]);
}

template <typename To, typename From>
inline decltype(bit_get<To>(std::declval<From *>()))
bit_get(From *ptr, ptrdiff_t offset) {
	return bit_get<To>(
		reinterpret_cast<From *>(reinterpret_cast<uintptr_t>(ptr) + offset));
}

template <typename To, typename From>
inline decltype(bit_get<To>(std::declval<From *>()))
bit_aligned_get(From *ptr, ptrdiff_t ix) {
	return bit_get<To>(ptr, ix * sizeof(To));
}

// bit_set to generic_ptr

template <typename From>
inline void bit_set(generic_ptr ptr, const From &val) {
	memcpy(ptr, &val, sizeof(From));
}

template <typename From>
inline void bit_set(generic_ptr ptr, ptrdiff_t offset, const From &val) {
	bit_set<From>(ptr + offset, val);
}

template <typename From>
inline void bit_aligned_set(generic_ptr ptr, ptrdiff_t ix, const From &val) {
	bit_set<From>(ptr + ix * sizeof(From), val);
}

// bit_set to To *

template <typename From, typename To>
inline void bit_set(To *ptr, const From &val) {
	memcpy(ptr, &val, sizeof(From));
}

template <typename From, typename To>
inline void bit_set(To *ptr, ptrdiff_t offset, const From &val) {
	bit_set<From>(
		reinterpret_cast<To *>(reinterpret_cast<uintptr_t>(ptr) + offset), val);
}

template <typename From, typename To>
inline void bit_aligned_set(To *ptr, ptrdiff_t ix, const From &val) {
	bit_set<From>(ptr, ix * sizeof(From), val);
}

// bit_cast

template <typename To, typename From>
inline enable_if_t<
	sizeof(To) == sizeof(From) && std::is_trivially_copyable<From>::value &&
		std::is_trivially_copyable<To>::value,
	decltype(bit_get<To>(std::declval<const From *>()))>
bit_cast(const From &src) {
	return bit_get<To>(&src);
}

// bit_eq

inline bool bit_eq(const uchar *a, const uchar *b, size_t size) {
	size_t i = 0;
	if (size >= sizeof(uintptr_t)) {
		for (;;) {
			if (bit_get<uintptr_t>(a + i) != bit_get<uintptr_t>(b + i)) {
				return false;
			}
			i += sizeof(uintptr_t);
			if (size - i < sizeof(uintptr_t)) {
				break;
			}
		}
	}
	for (; i < size; ++i) {
		if (a[i] != b[i]) {
			return false;
		}
	}
	return true;
}

inline bool bit_eq(generic_ptr a, generic_ptr b, size_t size) {
	return bit_eq(a.as<const uchar *>(), b.as<const uchar *>(), size);
}

inline bool
bit_eq(const uchar *a, const uchar *b, const uchar *mask, size_t size) {
	size_t i = 0;
	if (size >= sizeof(uintptr_t)) {
		for (;;) {
			auto m = bit_get<uintptr_t>(mask + i);
			if ((bit_get<uintptr_t>(a + i) & m) !=
				(bit_get<uintptr_t>(b + i) & m)) {
				return false;
			}
			i += sizeof(uintptr_t);
			if (size - i < sizeof(uintptr_t)) {
				break;
			}
		}
	}
	for (; i < size; ++i) {
		if ((a[i] & mask[i]) != (b[i] & mask[i])) {
			return false;
		}
	}
	return true;
}

inline bool
bit_eq(generic_ptr a, generic_ptr b, generic_ptr mask, size_t size) {
	return bit_eq(
		a.as<const uchar *>(),
		b.as<const uchar *>(),
		mask.as<const uchar *>(),
		size);
}

// bit_contains

inline bool bit_contains(
	const uchar *masked_byts,
	const uchar *mask,
	const uchar *byts,
	size_t size) {
	size_t i = 0;
	if (size >= sizeof(uintptr_t)) {
		for (;;) {
			if ((bit_get<uintptr_t>(byts + i) & bit_get<uintptr_t>(mask + i)) !=
				bit_get<uintptr_t>(masked_byts + i)) {
				return false;
			}
			i += sizeof(uintptr_t);
			if (size - i < sizeof(uintptr_t)) {
				break;
			}
		}
	}
	for (; i < size; ++i) {
		if ((byts[i] & mask[i]) != masked_byts[i]) {
			return false;
		}
	}
	return true;
}

inline bool bit_contains(
	generic_ptr masked_byts, generic_ptr mask, generic_ptr byts, size_t size) {
	return bit_eq(
		byts.as<const uchar *>(),
		masked_byts.as<const uchar *>(),
		mask.as<const uchar *>(),
		size);
}

} // namespace rua

#endif
