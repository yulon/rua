#ifndef _RUA_BIT_HPP
#define _RUA_BIT_HPP

#include "generic_ptr.hpp"
#include "macros.hpp"
#include "types/traits.hpp"
#include "types/util.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace rua {

// bit_as from generic_ptr

template <typename To>
RUA_FORCE_INLINE To &bit_as(generic_ptr ptr) {
	return *ptr.as<To *>();
}

template <typename To>
RUA_FORCE_INLINE To &bit_as(generic_ptr ptr, ptrdiff_t offset) {
	return bit_as<To>(ptr + offset);
}

template <typename To>
RUA_FORCE_INLINE To &bit_aligned_as(generic_ptr ptr, ptrdiff_t ix) {
	return bit_as<To>(ptr, ix * sizeof(To));
}

// bit_as from From *

template <
	typename To,
	typename From,
	typename Result =
		conditional_t<std::is_const<From>::value, add_const_t<To>, To>>
RUA_FORCE_INLINE Result &bit_as(From *ptr) {
	return *reinterpret_cast<Result *>(ptr);
}

template <
	typename To,
	typename From,
	typename Result =
		conditional_t<std::is_const<From>::value, add_const_t<To>, To>>
RUA_FORCE_INLINE Result &bit_as(From *ptr, ptrdiff_t offset) {
	return *reinterpret_cast<Result *>(
		reinterpret_cast<From *>(reinterpret_cast<uintptr_t>(ptr) + offset));
}

template <
	typename To,
	typename From,
	typename Result =
		conditional_t<std::is_const<From>::value, add_const_t<To>, To>>
RUA_FORCE_INLINE Result &bit_aligned_as(From *ptr, ptrdiff_t ix) {
	return *reinterpret_cast<Result *>(reinterpret_cast<From *>(
		reinterpret_cast<uintptr_t>(ptr) + ix * sizeof(To)));
}

// bit_get from generic_ptr

template <typename To>
RUA_FORCE_INLINE enable_if_t<(alignof(To) > 1), To> bit_get(generic_ptr ptr) {
	alignas(alignof(To)) char sto[sizeof(To)];
	memcpy(&sto[0], ptr, sizeof(To));
	return *reinterpret_cast<To *>(&sto[0]);
}

template <typename To>
RUA_FORCE_INLINE enable_if_t<(alignof(To) == 1), const remove_cv_t<To> &>
bit_get(generic_ptr ptr) {
	return *ptr.as<const remove_cv_t<To> *>();
}

template <typename To>
RUA_FORCE_INLINE decltype(bit_get<To>(nullptr))
bit_get(generic_ptr ptr, ptrdiff_t offset) {
	return bit_get<To>(ptr + offset);
}

template <typename To>
RUA_FORCE_INLINE decltype(bit_get<To>(nullptr))
bit_aligned_get(generic_ptr ptr, ptrdiff_t ix) {
	return bit_get<To>(ptr + ix * sizeof(To));
}

// bit_get from From *

template <typename To, typename From>
RUA_FORCE_INLINE enable_if_t<(alignof(To) > 1), To> bit_get(From *ptr) {
	alignas(alignof(To)) char sto[sizeof(To)];
	memcpy(&sto[0], reinterpret_cast<const void *>(ptr), sizeof(To));
	return *reinterpret_cast<To *>(&sto[0]);
}

template <typename To, typename From, typename ConstTo = add_const_t<To>>
RUA_FORCE_INLINE enable_if_t<(alignof(To) == 1), ConstTo &> bit_get(From *ptr) {
	return *reinterpret_cast<ConstTo *>(ptr);
}

template <typename To, typename From>
RUA_FORCE_INLINE decltype(bit_get<To>(std::declval<From *>()))
bit_get(From *ptr, ptrdiff_t offset) {
	return bit_get<To>(
		reinterpret_cast<From *>(reinterpret_cast<uintptr_t>(ptr) + offset));
}

template <
	typename To,
	typename From,
	typename Result =
		conditional_t<std::is_const<From>::value, add_const_t<To>, To>>
RUA_FORCE_INLINE decltype(bit_get<To>(std::declval<From *>()))
bit_aligned_get(From *ptr, ptrdiff_t ix) {
	return bit_get<To>(ptr, ix * sizeof(To));
}

// bit_set to generic_ptr

template <typename From>
RUA_FORCE_INLINE void bit_set(generic_ptr ptr, const From &val) {
	memcpy(ptr, &val, sizeof(From));
}

template <typename From>
RUA_FORCE_INLINE void
bit_set(generic_ptr ptr, ptrdiff_t offset, const From &val) {
	bit_set<From>(ptr + offset, val);
}

template <typename From>
RUA_FORCE_INLINE void
bit_aligned_set(generic_ptr ptr, ptrdiff_t ix, const From &val) {
	bit_set<From>(ptr + ix * sizeof(From), val);
}

// bit_set to To *

template <typename From, typename To>
RUA_FORCE_INLINE void bit_set(To *ptr, const From &val) {
	memcpy(reinterpret_cast<void *>(ptr), &val, sizeof(From));
}

template <typename From, typename To>
RUA_FORCE_INLINE void bit_set(To *ptr, ptrdiff_t offset, const From &val) {
	bit_set<From>(
		reinterpret_cast<To *>(reinterpret_cast<uintptr_t>(ptr) + offset), val);
}

template <typename From, typename To>
RUA_FORCE_INLINE void bit_aligned_set(To *ptr, ptrdiff_t ix, const From &val) {
	bit_set<From>(ptr, ix * sizeof(From), val);
}

// bit_cast

template <typename To, typename From>
RUA_FORCE_INLINE decltype(bit_get<To>(std::declval<const From *>()))
bit_cast(const From &src) {
	return bit_get<To>(&src);
}

// bit_eq

RUA_FORCE_INLINE bool bit_eq(const byte *a, const byte *b, size_t size) {
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

RUA_FORCE_INLINE bool bit_eq(generic_ptr a, generic_ptr b, size_t size) {
	return bit_eq(a.as<const byte *>(), b.as<const byte *>(), size);
}

RUA_FORCE_INLINE bool
bit_eq(const byte *a, const byte *b, const byte *mask, size_t size) {
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

RUA_FORCE_INLINE bool
bit_eq(generic_ptr a, generic_ptr b, generic_ptr mask, size_t size) {
	return bit_eq(
		a.as<const byte *>(),
		b.as<const byte *>(),
		mask.as<const byte *>(),
		size);
}

// bit_has

RUA_FORCE_INLINE bool
bit_has(const byte *src, const byte *masked, const byte *mask, size_t size) {
	size_t i = 0;
	if (size >= sizeof(uintptr_t)) {
		for (;;) {
			if ((bit_get<uintptr_t>(src + i) & bit_get<uintptr_t>(mask + i)) !=
				bit_get<uintptr_t>(masked + i)) {
				return false;
			}
			i += sizeof(uintptr_t);
			if (size - i < sizeof(uintptr_t)) {
				break;
			}
		}
	}
	for (; i < size; ++i) {
		if ((src[i] & mask[i]) != masked[i]) {
			return false;
		}
	}
	return true;
}

RUA_FORCE_INLINE bool
bit_has(generic_ptr src, generic_ptr masked, generic_ptr mask, size_t size) {
	return bit_eq(
		src.as<const byte *>(),
		masked.as<const byte *>(),
		mask.as<const byte *>(),
		size);
}

} // namespace rua

#endif
