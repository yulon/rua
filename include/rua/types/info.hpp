#ifndef _RUA_TYPES_INFO_HPP
#define _RUA_TYPES_INFO_HPP

#include "traits.hpp"
#include "util.hpp"

#include <string>

#ifdef RUA_RTTI
#include <typeindex>
#include <typeinfo>
#endif

namespace rua {

struct type_info_t;

struct type_id_t {
	type_info_t &(*info)();
};

RUA_FORCE_INLINE bool operator==(type_id_t a, type_id_t b) {
	return a.info == b.info;
}

RUA_FORCE_INLINE bool operator!=(type_id_t a, type_id_t b) {
	return a.info != b.info;
}

struct type_info_t {
	const type_id_t id;

	const size_t size;

	const size_t align;

	const bool is_trivial;

	std::string &(*const desc)();

	void (*const del)(void *);

	uintptr_t (*const new_copy)(const void *src);

	void (*const dtor)(void *);

	void (*const copy_ctor)(void *ptr, const void *src);

	void copy(void *dest, const void *src) {
		if (dtor) {
			dtor(dest);
		}
		copy_ctor(dest, src);
	}

	void (*const move_ctor)(void *ptr, void *src);

	void move(void *dest, void *src) {
		if (dtor) {
			dtor(dest);
		}
		move_ctor(dest, src);
	}

#ifdef RUA_RTTI
	const std::type_info &(*const std_id)();
#endif
};

RUA_FORCE_INLINE bool operator==(type_id_t a, const type_info_t &b) {
	return a.info == b.id.info;
}

RUA_FORCE_INLINE bool operator!=(type_id_t a, const type_info_t &b) {
	return a.info != b.id.info;
}

RUA_FORCE_INLINE bool operator==(const type_info_t &a, type_id_t b) {
	return a.id.info == b.info;
}

RUA_FORCE_INLINE bool operator!=(const type_info_t &a, type_id_t b) {
	return a.id.info != b.info;
}

#ifdef RUA_RTTI

RUA_FORCE_INLINE bool operator==(type_id_t a, const std::type_info &b) {
	return a.info().std_id() == b;
}

RUA_FORCE_INLINE bool operator!=(type_id_t a, const std::type_info &b) {
	return a.info().std_id() != b;
}

RUA_FORCE_INLINE bool operator==(const std::type_info &a, type_id_t b) {
	return a == b.info().std_id();
}

RUA_FORCE_INLINE bool operator!=(const std::type_info &a, type_id_t b) {
	return a != b.info().std_id();
}

RUA_FORCE_INLINE bool operator==(type_id_t a, std::type_index b) {
	return std::type_index(a.info().std_id()) == b;
}

RUA_FORCE_INLINE bool operator!=(type_id_t a, std::type_index b) {
	return std::type_index(a.info().std_id()) != b;
}

RUA_FORCE_INLINE bool operator==(std::type_index a, type_id_t b) {
	return a == std::type_index(b.info().std_id());
}

RUA_FORCE_INLINE bool operator!=(std::type_index a, type_id_t b) {
	return a != std::type_index(b.info().std_id());
}

RUA_FORCE_INLINE bool
operator==(const type_info_t &a, const std::type_info &b) {
	return a.std_id() == b;
}

RUA_FORCE_INLINE bool
operator!=(const type_info_t &a, const std::type_info &b) {
	return a.std_id() != b;
}

RUA_FORCE_INLINE bool
operator==(const std::type_info &a, const type_info_t &b) {
	return a == b.std_id();
}

RUA_FORCE_INLINE bool
operator!=(const std::type_info &a, const type_info_t &b) {
	return a != b.std_id();
}

RUA_FORCE_INLINE bool operator==(const type_info_t &a, std::type_index b) {
	return std::type_index(a.std_id()) == b;
}

RUA_FORCE_INLINE bool operator!=(const type_info_t &a, std::type_index b) {
	return std::type_index(a.std_id()) != b;
}

RUA_FORCE_INLINE bool operator==(std::type_index a, const type_info_t &b) {
	return a == std::type_index(b.std_id());
}

RUA_FORCE_INLINE bool operator!=(std::type_index a, const type_info_t &b) {
	return a != std::type_index(b.std_id());
}

#endif

template <typename T, typename = void>
struct _type_del {
	static constexpr std::nullptr_t value = nullptr;
};

template <typename T>
struct _type_del<T, enable_if_t<(size_of<T>::value > 0)>> {
	static void value(void *ptr) {
		delete reinterpret_cast<T *>(ptr);
	}
};

template <typename T, typename = void>
struct _type_new_copy {
	static constexpr std::nullptr_t value = nullptr;
};

template <typename T>
struct _type_new_copy<T, enable_if_t<std::is_copy_constructible<T>::value>> {
	static uintptr_t value(const void *src) {
		return reinterpret_cast<uintptr_t>(
			new T(*reinterpret_cast<const T *>(src)));
	}
};

template <typename T, typename = void>
struct _type_dtor {
	static constexpr std::nullptr_t value = nullptr;
};

template <typename T>
struct _type_dtor<
	T,
	enable_if_t<size_of<T>::value && !std::is_trivial<T>::value>> {
	static void value(void *ptr) {
		reinterpret_cast<T *>(ptr)->~T();
	}
};

template <typename T, typename = void>
struct _type_copy_ctor {
	static constexpr std::nullptr_t value = nullptr;
};

template <typename T>
struct _type_copy_ctor<T, enable_if_t<std::is_copy_constructible<T>::value>> {
	static void value(void *ptr, const void *src) {
		new (reinterpret_cast<remove_cv_t<T> *>(ptr))
			T(*reinterpret_cast<const T *>(src));
	};
};

template <typename T, typename = void>
struct _type_move_ctor {
	static constexpr std::nullptr_t value = nullptr;
};

template <typename T>
struct _type_move_ctor<T, enable_if_t<std::is_move_constructible<T>::value>> {
	static void value(void *ptr, void *src) {
		new (reinterpret_cast<remove_cv_t<T> *>(ptr))
			T(std::move(*reinterpret_cast<T *>(src)));
	}
};

template <typename T>
inline std::string &_type_desc() {
	static std::string desc;
	return desc;
}

#ifdef RUA_RTTI
template <typename T>
inline const std::type_info &_type_std_info() {
	return typeid(T);
}
#endif

template <typename T>
inline type_info_t &type_info() {
	static type_info_t inf{type_id_t{&type_info<T>},
						   size_of<T>::value,
						   align_of<T>::value,
						   std::is_trivial<T>::value,
						   &_type_desc<T>,
						   _type_del<T>::value,
						   _type_new_copy<T>::value,
						   _type_dtor<T>::value,
						   _type_copy_ctor<T>::value,
						   _type_move_ctor<T>::value
#ifdef RUA_RTTI
						   ,
						   &_type_std_info<T>
#endif
	};
	return inf;
}

template <typename T>
inline constexpr type_id_t type_id() {
	return type_id_t{&type_info<T>};
}

} // namespace rua

#endif
