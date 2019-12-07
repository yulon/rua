#ifndef _RUA_BIT_HPP
#define _RUA_BIT_HPP

#include "byte.hpp"
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
		!std::is_same<NCTo, byte>::value,
	To>
bit_get(generic_ptr ptr) {
	alignas(alignof(To)) char sto[sizeof(To)];
	memcpy(&sto, ptr, sizeof(To));
	return reinterpret_cast<To &>(sto);
}

template <typename To, typename NCTo = remove_cv_t<To>>
RUA_FORCE_INLINE enable_if_t<
	std::is_trivial<To>::value &&
		(std::is_same<NCTo, char>::value || std::is_same<NCTo, byte>::value),
	const NCTo &>
bit_get(generic_ptr ptr) {
	return *ptr.as<To *>();
}

template <typename To, typename NCTo = remove_cv_t<To>>
RUA_FORCE_INLINE enable_if_t<
	std::is_trivial<To>::value,
	conditional_t<
		!std::is_same<NCTo, char>::value && !std::is_same<NCTo, byte>::value,
		To,
		const NCTo &>>
bit_get(generic_ptr ptr, ptrdiff_t offset) {
	return bit_get<To>(ptr + offset);
}

template <typename To, typename NCTo = remove_cv_t<To>>
RUA_FORCE_INLINE enable_if_t<
	std::is_trivial<To>::value,
	conditional_t<
		!std::is_same<NCTo, char>::value && !std::is_same<NCTo, byte>::value,
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
		 std::is_same<NCFrom, byte>::value),
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
		(std::is_same<NCTo, char>::value || std::is_same<NCTo, byte>::value) &&
		!std::is_same<NCFrom, char>::value &&
		!std::is_same<NCFrom, byte>::value,
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

// bit_eq

template <typename APtr, typename BPtr>
RUA_FORCE_INLINE enable_if_t<
	std::is_convertible<APtr, generic_ptr>::value &&
		std::is_convertible<BPtr, generic_ptr>::value,
	bool>
bit_eq(APtr a, BPtr b, size_t size) {
	size_t i = 0;
	while (i < size) {
		auto left_sz = size - i;

		if (left_sz > sizeof(uintmax_t)) {
			if (bit_get<uintmax_t>(a, i) != bit_get<uintmax_t>(b, i)) {
				return false;
			}
			i += sizeof(uintmax_t);
			continue;
		}

		switch (left_sz) {
		case 8:
			if (bit_get<uint64_t>(a, i) != bit_get<uint64_t>(b, i)) {
				return false;
			}
			break;

		case 7:
			if (bit_get<byte>(a, i + 6) != bit_get<byte>(b, i + 6)) {
				return false;
			}
			RUA_FALLTHROUGH;

		case 6:
			if (bit_get<uint32_t>(a, i) != bit_get<uint32_t>(b, i) ||
				bit_get<uint16_t>(a, i + 4) != bit_get<uint16_t>(b, i + 4)) {
				return false;
			}
			break;

		case 5:
			if (bit_get<byte>(a, i + 4) != bit_get<byte>(b, i + 4)) {
				return false;
			}
			RUA_FALLTHROUGH;

		case 4:
			if (bit_get<uint32_t>(a, i) != bit_get<uint32_t>(b, i)) {
				return false;
			}
			break;

		case 3:
			if (bit_get<byte>(a, i + 2) != bit_get<byte>(b, i + 2)) {
				return false;
			}
			RUA_FALLTHROUGH;

		case 2:
			if (bit_get<uint16_t>(a, i) != bit_get<uint16_t>(b, i)) {
				return false;
			}
			break;

		case 1:
			if (bit_get<byte>(a, i) != bit_get<byte>(b, i)) {
				return false;
			}
			break;

		default:
			for (size_t j = 0; j < left_sz; ++j) {
				if (bit_get<byte>(a, i + j) != bit_get<byte>(b, i + j)) {
					return false;
				}
			}
		}

		i += left_sz;
	}
	return true;
}

template <typename APtr, typename BPtr, typename MaskPtr>
RUA_FORCE_INLINE enable_if_t<
	std::is_convertible<APtr, generic_ptr>::value &&
		std::is_convertible<BPtr, generic_ptr>::value &&
		std::is_convertible<MaskPtr, generic_ptr>::value,
	bool>
bit_eq(APtr a, BPtr b, MaskPtr mask, size_t size) {
	size_t i = 0;
	while (i < size) {
		auto left_sz = size - i;

		if (left_sz > sizeof(uintmax_t)) {
			auto m = bit_get<uintmax_t>(mask, i);
			if ((bit_get<uintmax_t>(a, i) & m) !=
				(bit_get<uintmax_t>(b, i) & m)) {
				return false;
			}
			i += sizeof(uintmax_t);
			continue;
		}

		switch (left_sz) {
		case 8: {
			auto m = bit_get<uint64_t>(mask, i);
			if ((bit_get<uint64_t>(a, i) & m) !=
				(bit_get<uint64_t>(b, i) & m)) {
				return false;
			}
			break;
		}

		case 7: {
			auto m = bit_get<byte>(mask, i + 6);
			if ((bit_get<byte>(a, i + 6) & m) !=
				(bit_get<byte>(b, i + 6) & m)) {
				return false;
			}
			RUA_FALLTHROUGH;
		}

		case 6: {
			auto m = bit_get<uint32_t>(mask, i);
			if ((bit_get<uint32_t>(a, i) & m) !=
				(bit_get<uint32_t>(b, i) & m)) {
				return false;
			}
			m = bit_get<uint16_t>(mask, i + 4);
			if ((bit_get<uint16_t>(a, i + 4) & m) !=
				(bit_get<uint16_t>(b, i + 4) & m)) {
				return false;
			}
			break;
		}

		case 5: {
			auto m = bit_get<byte>(mask, i + 4);
			if ((bit_get<byte>(a, i + 4) & m) !=
				(bit_get<byte>(b, i + 4) & m)) {
				return false;
			}
			RUA_FALLTHROUGH;
		}

		case 4: {
			auto m = bit_get<uint32_t>(mask, i);
			if ((bit_get<uint32_t>(a, i) & m) !=
				(bit_get<uint32_t>(b, i) & m)) {
				return false;
			}
			break;
		}

		case 3: {
			auto m = bit_get<byte>(mask, i + 2);
			if ((bit_get<byte>(a, i + 2) & m) !=
				(bit_get<byte>(b, i + 2) & m)) {
				return false;
			}
			RUA_FALLTHROUGH;
		}

		case 2: {
			auto m = bit_get<uint16_t>(mask, i);
			if ((bit_get<uint16_t>(a, i) & m) !=
				(bit_get<uint16_t>(b, i) & m)) {
				return false;
			}
			break;
		}

		case 1: {
			auto m = bit_get<byte>(mask, i);
			if ((bit_get<byte>(a, i) & m) != (bit_get<byte>(b, i) & m)) {
				return false;
			}
			break;
		}

		default:
			for (size_t j = 0; j < left_sz; ++j) {
				auto m = bit_get<byte>(mask, i + j);
				if ((bit_get<byte>(a, i + j) & m) !=
					(bit_get<byte>(b, i + j) & m)) {
					return false;
				}
			}
		}

		i += left_sz;
	}
	return true;
}

// bit_has

template <typename FromPtr, typename ValPtr, typename ValMaskPtr>
RUA_FORCE_INLINE enable_if_t<
	std::is_convertible<FromPtr, generic_ptr>::value &&
		std::is_convertible<ValPtr, generic_ptr>::value &&
		std::is_convertible<ValMaskPtr, generic_ptr>::value,
	bool>
bit_has(FromPtr from, ValPtr val, ValMaskPtr val_mask, size_t size) {
	size_t i = 0;
	while (i < size) {
		auto left_sz = size - i;

		if (left_sz > sizeof(uintmax_t)) {
			if ((bit_get<uintmax_t>(from, i) &
				 bit_get<uintmax_t>(val_mask, i)) !=
				bit_get<uintmax_t>(val, i)) {
				return false;
			}
			i += sizeof(uintmax_t);
			continue;
		}

		switch (left_sz) {
		case 8:
			if ((bit_get<uint64_t>(from, i) & bit_get<uint64_t>(val_mask, i)) !=
				bit_get<uint64_t>(val, i)) {
				return false;
			}
			break;

		case 7:
			if ((bit_get<byte>(from, i + 6) & bit_get<byte>(val_mask, i + 6)) !=
				bit_get<byte>(val, i + 6)) {
				return false;
			}
			RUA_FALLTHROUGH;

		case 6:
			if ((bit_get<uint32_t>(from, i) & bit_get<uint32_t>(val_mask, i)) !=
					bit_get<uint32_t>(val, i) ||
				(bit_get<uint16_t>(from, i + 4) &
				 bit_get<uint16_t>(val_mask, i + 4)) !=
					bit_get<uint16_t>(val, i + 4)) {
				return false;
			}
			break;

		case 5:
			if ((bit_get<byte>(from, i + 4) & bit_get<byte>(val_mask, i + 4)) !=
				bit_get<byte>(val, i + 4)) {
				return false;
			}
			RUA_FALLTHROUGH;

		case 4:
			if ((bit_get<uint32_t>(from, i) & bit_get<uint32_t>(val_mask, i)) !=
				bit_get<uint32_t>(val, i)) {
				return false;
			}
			break;

		case 3:
			if ((bit_get<byte>(from, i + 2) & bit_get<byte>(val_mask, i + 2)) !=
				bit_get<byte>(val, i + 2)) {
				return false;
			}
			RUA_FALLTHROUGH;

		case 2:
			if ((bit_get<uint16_t>(from, i) & bit_get<uint16_t>(val_mask, i)) !=
				bit_get<uint16_t>(val, i)) {
				return false;
			}
			break;

		case 1:
			if ((bit_get<byte>(from, i) & bit_get<byte>(val_mask, i)) !=
				bit_get<byte>(val, i)) {
				return false;
			}
			break;

		default:
			for (size_t j = 0; j < left_sz; ++j) {
				if ((bit_get<byte>(from, i + j) &
					 bit_get<byte>(val_mask, i + j)) !=
					bit_get<byte>(val, i + j)) {
					return false;
				}
			}
		}

		i += left_sz;
	}
	return true;
}

} // namespace rua

#endif
