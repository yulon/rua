#ifndef _RUA_TYPE_TRAITS_TYPE_INFO_HPP
#define _RUA_TYPE_TRAITS_TYPE_INFO_HPP

#include "measures.hpp"
#include "std_patch.hpp"

#include "../macros.hpp"

#include <cstdint>
#include <string>

#ifdef RUA_USING_RTTI
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

	void (*const del)(uintptr_t);

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

#ifdef RUA_USING_RTTI
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

#ifdef RUA_USING_RTTI

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

template <typename T>
inline enable_if_t<std::is_void<T>::value, void (*)(uintptr_t)> _type_del() {
	return nullptr;
}

template <typename T>
inline enable_if_t<!std::is_void<T>::value, void (*)(uintptr_t)> _type_del() {
	return [](uintptr_t ptr) { delete reinterpret_cast<T *>(ptr); };
}

template <typename T>
inline enable_if_t<
	std::is_copy_constructible<T>::value,
	uintptr_t (*)(const void *)>
_type_new_copy() {
	return [](const void *src) -> uintptr_t {
		return reinterpret_cast<uintptr_t>(
			new T(*reinterpret_cast<const T *>(src)));
	};
}

template <typename T>
inline enable_if_t<
	!std::is_copy_constructible<T>::value,
	uintptr_t (*)(const void *)>
_type_new_copy() {
	return nullptr;
}

template <typename T>
inline enable_if_t<
	std::is_void<T>::value || std::is_trivial<T>::value,
	void (*)(void *)>
_type_dtor() {
	return nullptr;
}

template <typename T>
inline enable_if_t<
	!std::is_void<T>::value && !std::is_trivial<T>::value,
	void (*)(void *)>
_type_dtor() {
	return [](void *ptr) { reinterpret_cast<T *>(ptr)->~T(); };
}

template <typename T>
inline enable_if_t<
	std::is_copy_constructible<T>::value,
	void (*)(void *, const void *)>
_type_copy_ctor() {
	return [](void *ptr, const void *src) {
		new (reinterpret_cast<remove_cv_t<T> *>(ptr))
			T(*reinterpret_cast<const T *>(src));
	};
}

template <typename T>
inline enable_if_t<
	!std::is_copy_constructible<T>::value,
	void (*)(void *, const void *)>
_type_copy_ctor() {
	return nullptr;
}

template <typename T>
inline enable_if_t<
	std::is_move_constructible<T>::value,
	void (*)(void *, void *)>
_type_move_ctor() {
	return [](void *ptr, void *src) {
		new (reinterpret_cast<std::remove_cv_t<T> *>(ptr))
			T(std::move(*reinterpret_cast<T *>(src)));
	};
}

template <typename T>
inline enable_if_t<
	!std::is_move_constructible<T>::value,
	void (*)(void *, void *)>
_type_move_ctor() {
	return nullptr;
}

template <typename T>
inline type_info_t &type_info() {
	static type_info_t inf{type_id_t{&type_info<T>},
						   size_of<T>::value,
						   align_of<T>::value,
						   std::is_trivial<T>::value,
						   []() -> std::string & {
							   static std::string desc;
							   return desc;
						   },
						   _type_del<T>(),
						   _type_new_copy<T>(),
						   _type_dtor<T>(),
						   _type_copy_ctor<T>(),
						   _type_move_ctor<T>()
#ifdef RUA_USING_RTTI
							   ,
						   []() -> const std::type_info & { return typeid(T); }
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
