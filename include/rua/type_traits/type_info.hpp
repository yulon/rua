#ifndef _RUA_TYPE_TRAITS_TYPE_INFO_HPP
#define _RUA_TYPE_TRAITS_TYPE_INFO_HPP

#include "size_of.hpp"

#include "../any_word.hpp"
#include "../macros.hpp"

#include <cstdint>
#include <string>
#include <type_traits>

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

	std::string &(*const desc)();

	void (*const del)(void *);

	void (*const dtor)(void *);

	void (*const dtor_from_any_word)(any_word);

	void (*const copy_ctor)(void *ptr, void *src);

	void copy(void *dest, void *src) {
		if (dtor) {
			dtor(dest);
		}
		copy_ctor(dest, src);
	}

	any_word (*const copy_from_any_word)(const any_word src);

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
inline typename std::enable_if<std::is_void<T>::value, void (*)(void *)>::type
_type_del() {
	return nullptr;
}

template <typename T>
inline typename std::enable_if<!std::is_void<T>::value, void (*)(void *)>::type
_type_del() {
	return [](void *ptr) { delete reinterpret_cast<T *>(ptr); };
}

template <typename T>
inline typename std::enable_if<
	std::is_void<T>::value || std::is_trivial<T>::value,
	void (*)(void *)>::type
_type_dtor() {
	return nullptr;
}

template <typename T>
inline typename std::enable_if<
	!std::is_void<T>::value && !std::is_trivial<T>::value,
	void (*)(void *)>::type
_type_dtor() {
	return [](void *ptr) { reinterpret_cast<T *>(ptr)->~T(); };
}

template <typename T>
inline typename std::enable_if<
	!std::is_void<T>::value && !std::is_integral<T>::value &&
		!std::is_pointer<T>::value &&
		!(size_of<T>::value <= sizeof(uintptr_t) && std::is_trivial<T>::value),
	void (*)(any_word)>::type
_type_dtor_from_any_word() {
	return [](any_word w) { w.destruct<T>(); };
}

template <typename T>
inline typename std::enable_if<
	std::is_void<T>::value || std::is_integral<T>::value ||
		std::is_pointer<T>::value ||
		(size_of<T>::value <= sizeof(uintptr_t) && std::is_trivial<T>::value),
	void (*)(any_word)>::type
_type_dtor_from_any_word() {
	return nullptr;
}

template <typename T>
inline typename std::enable_if<
	std::is_copy_constructible<T>::value,
	void (*)(void *, void *)>::type
_type_copy_ctor() {
	return [](void *ptr, void *src) {
		new (reinterpret_cast<T *>(ptr)) T(*reinterpret_cast<const T *>(src));
	};
}

template <typename T>
inline typename std::enable_if<
	!std::is_copy_constructible<T>::value,
	void (*)(void *, void *)>::type
_type_copy_ctor() {
	return nullptr;
}

template <typename T>
inline typename std::enable_if<
	std::is_copy_constructible<T>::value,
	any_word (*)(const any_word)>::type
_type_copy_from_any_word() {
	return [](const any_word src) -> any_word { return any_word(src.as<T>()); };
}

template <typename T>
inline typename std::enable_if<
	!std::is_copy_constructible<T>::value,
	any_word (*)(const any_word)>::type
_type_copy_from_any_word() {
	return nullptr;
}

template <typename T>
inline typename std::enable_if<
	std::is_move_constructible<T>::value,
	void (*)(void *, void *)>::type
_type_move_ctor() {
	return [](void *ptr, void *src) {
		new (reinterpret_cast<T *>(ptr))
			T(std::move(*reinterpret_cast<T *>(src)));
	};
}

template <typename T>
inline typename std::enable_if<
	!std::is_move_constructible<T>::value,
	void (*)(void *, void *)>::type
_type_move_ctor() {
	return nullptr;
}

template <typename T>
inline type_info_t &type_info() {
	static type_info_t inf{type_id_t{&type_info<T>},
						   []() -> std::string & {
							   static std::string desc;
							   return desc;
						   },
						   _type_del<T>(),
						   _type_dtor<T>(),
						   _type_dtor_from_any_word<T>(),
						   _type_copy_ctor<T>(),
						   _type_copy_from_any_word<T>(),
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
