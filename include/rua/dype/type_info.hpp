#ifndef _rua_dype_type_info_hpp
#define _rua_dype_type_info_hpp

#include "../string/join.hpp"
#include "../string/len.hpp"
#include "../string/view.hpp"
#include "../util.hpp"

#include <cassert>
#include <string>

#ifdef RUA_HAS_RTTI
#include <typeindex>
#endif

namespace rua {

template <typename T>
inline constexpr const char *_func_name() RUA_NOEXCEPT {
#if defined(__GNUC__) || defined(__clang__)
	return __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
	return __FUNCSIG__;
#endif
}

template <typename T>
inline constexpr size_t _func_name_prefix_len() RUA_NOEXCEPT {
#if defined(__GNUC__) || defined(__clang__)
	return c_str_len("constexpr const char* rua::_func_name() [with T = ");
#elif defined(_MSC_VER)
	return c_str_len("const char *__cdecl rua::_func_name<");
#endif
}

template <typename T>
inline constexpr size_t _func_name_suffix_len() RUA_NOEXCEPT {
#if defined(__GNUC__) || defined(__clang__)
	return c_str_len("]");
#elif defined(_MSC_VER)
#ifdef RUA_HAS_EXCEPTIONS
	return c_str_len(">(void) noexcept");
#else
	return c_str_len(">(void)");
#endif
#endif
}

template <typename T>
inline constexpr string_view
_type_name(size_t func_name_prefix_len) RUA_NOEXCEPT {
	return string_view(
		_func_name<T>() + func_name_prefix_len,
		c_str_len(_func_name<T>()) -
			(func_name_prefix_len + _func_name_suffix_len<T>()));
}

template <typename T>
inline constexpr string_view type_name() RUA_NOEXCEPT {
	return _type_name<T>(_func_name_prefix_len<T>());
}

class type_info {
public:
	constexpr type_info(std::nullptr_t = nullptr) RUA_NOEXCEPT : $tab(nullptr) {
	}

	template <typename T>
	constexpr explicit type_info(in_place_type_t<T>) RUA_NOEXCEPT
		: $tab(&$table<T>()) {}

	constexpr explicit type_info(in_place_type_t<void>) RUA_NOEXCEPT
		: $tab(nullptr) {}

	operator bool() const RUA_NOEXCEPT {
		return $tab;
	}

	bool operator==(const type_info &target) const RUA_NOEXCEPT {
		return $tab == target.$tab;
	}

	bool operator!=(const type_info &target) const RUA_NOEXCEPT {
		return $tab != target.$tab;
	}

	size_t size() const RUA_NOEXCEPT {
		return $tab ? $tab->size : 0;
	}

	size_t align() const RUA_NOEXCEPT {
		return $tab ? $tab->align : 0;
	}

	bool
	is_placeable(size_t storage_sz, size_t storage_align) const RUA_NOEXCEPT {
		return !$tab || RUA_IS_PLACEABLE(
							$tab->size, $tab->align, storage_sz, storage_align);
	}

	bool is_trivial() const RUA_NOEXCEPT {
		return $tab ? $tab->is_trivial : false;
	}

	string_view name() const RUA_NOEXCEPT {
		return $tab ? $tab->name() : type_name<void>();
	}

	size_t hash_code() const RUA_NOEXCEPT {
		return static_cast<size_t>(reinterpret_cast<uintptr_t>($tab));
	}

	void destruct(void *ptr) const {
		assert($tab);
		if (!$tab->dtor) {
			return;
		}
		$tab->dtor(ptr);
	}

	void dealloc(void *ptr) const {
		assert($tab);
		assert($tab->del);
		$tab->del(ptr);
	}

	template <size_t Size, size_t Align>
	void destruct_storage(void *ptr) const {
		if (is_placeable(Size, Align)) {
			destruct(ptr);
			return;
		}
		dealloc(ptr);
	}

	bool is_copyable() const RUA_NOEXCEPT {
		return $tab && $tab->copy_ctor;
	}

	void copy_to(void *dest, const void *src) const {
		assert($tab);
		assert($tab->copy_ctor);
		$tab->copy_ctor(dest, src);
	}

	void *copy_to_new(const void *src) const {
		assert($tab);
		assert($tab->copy_new);
		return $tab->copy_new(src);
	}

	template <size_t Size, size_t Align>
	void copy_to_storage(void *dest, const void *src) const {
		if (is_placeable(Size, Align)) {
			copy_to(dest, src);
			return;
		}
		*reinterpret_cast<void **>(dest) =
			copy_to_new(*reinterpret_cast<const void *const *>(src));
	}

	template <
		size_t DestSize,
		size_t DestAlign,
		size_t SrcSize,
		size_t SrcAlign>
	void copy_to_storage(void *dest, const void *src) const {
		if (!is_placeable(SrcSize, SrcAlign)) {
			src = *reinterpret_cast<const void *const *>(src);
		}
		if (is_placeable(DestSize, DestAlign)) {
			copy_to(dest, src);
			return;
		}
		*reinterpret_cast<void **>(dest) = copy_to_new(src);
	}

	bool is_moveable() const RUA_NOEXCEPT {
		return $tab && $tab->move_ctor;
	}

	void move_to(void *dest, void *src) const {
		assert($tab);
		assert($tab->move_ctor);
		$tab->move_ctor(dest, src);
	}

	void *move_to_new(void *src) const {
		assert($tab);
		assert($tab->move_new);
		return $tab->move_new(src);
	}

	template <size_t Size, size_t Align>
	void move_to_storage(void *dest, void *src) const {
		if (is_placeable(Size, Align)) {
			move_to(dest, src);
			return;
		}
		*reinterpret_cast<void **>(dest) = *reinterpret_cast<void **>(src);
	}

	template <
		size_t DestSize,
		size_t DestAlign,
		size_t SrcSize,
		size_t SrcAlign>
	void move_to_storage(void *dest, void *src) const {
		if (is_placeable(SrcSize, SrcAlign)) {
			if (is_placeable(DestSize, DestAlign)) {
				move_to(dest, src);
				return;
			}
			*reinterpret_cast<void **>(dest) = move_to_new(src);
			return;
		}
		src = *reinterpret_cast<void **>(src);
		if (is_placeable(DestSize, DestAlign)) {
			move_to(dest, src);
			dealloc(src);
			return;
		}
		*reinterpret_cast<void **>(dest) = src;
	}

	bool is_convertable_to_bool() const RUA_NOEXCEPT {
		return $tab && $tab->to_bool;
	}

	bool convert_to_bool(void *ptr) const {
		assert($tab);
		assert($tab->to_bool);
		return $tab->to_bool(ptr);
	}

	bool equal(const void *a, const void *b) const {
		assert($tab);
		return a == b || ($tab->eq && $tab->eq(a, b));
	}

#ifdef RUA_HAS_RTTI
	const std::type_info &std_id() const {
		return $tab ? $tab->std_id() : typeid(void);
	}
#endif

private:
	struct $table_t {
		const size_t size;

		const size_t align;

		const bool is_trivial;

		string_view (*const name)();

		void (*const dtor)(void *);

		void (*const del)(void *);

		void (*const copy_ctor)(void *ptr, const void *src);

		void *(*const copy_new)(const void *src);

		void (*const move_ctor)(void *ptr, void *src);

		void *(*const move_new)(void *src);

		bool (*const to_bool)(void *);

		bool (*const eq)(const void *a, const void *b);

#ifdef RUA_HAS_RTTI
		const std::type_info &(*const std_id)();
#endif
	};

	template <typename T, typename = void>
	struct $dtor {
		static constexpr std::nullptr_t value = nullptr;
	};

	template <typename T>
	struct $dtor<
		T,
		enable_if_t<
			size_of<T>::value && !std::is_trivial<T>::value &&
			!std::is_reference<T>::value>> {
		static void value(void *ptr) {
			reinterpret_cast<remove_cv_t<T> *>(ptr)->~T();
		}
	};

	template <typename T, typename = void>
	struct $del {
		static constexpr std::nullptr_t value = nullptr;
	};

	template <typename T>
	struct $del<
		T,
		enable_if_t<(size_of<T>::value > 0) && !std::is_reference<T>::value>> {
		static void value(void *ptr) {
			delete reinterpret_cast<remove_cv_t<T> *>(ptr);
		}
	};

	template <typename T>
	struct $is_initializer_list : std::false_type {};

	template <typename E>
	struct $is_initializer_list<std::initializer_list<E>> : std::true_type {};

	template <typename T, typename = void>
	struct $copy_ctor {
		static constexpr std::nullptr_t value = nullptr;
	};

	template <typename T>
	struct $copy_ctor<
		T,
		enable_if_t<
			std::is_copy_constructible<T>::value &&
			!$is_initializer_list<T>::value && !std::is_reference<T>::value>> {
		static void value(void *ptr, const void *src) {
			construct(
				*reinterpret_cast<remove_cv_t<T> *>(ptr),
				*reinterpret_cast<const T *>(src));
		};
	};

	template <typename T, typename = void>
	struct $copy_new {
		static constexpr std::nullptr_t value = nullptr;
	};

	template <typename T>
	struct $copy_new<
		T,
		enable_if_t<
			std::is_copy_constructible<T>::value &&
			!$is_initializer_list<T>::value && !std::is_reference<T>::value>> {
		static void *value(const void *src) {
			return new remove_cv_t<T>(*reinterpret_cast<const T *>(src));
		}
	};

	template <typename T, typename = void>
	struct $move_ctor {
		static constexpr std::nullptr_t value = nullptr;
	};

	template <typename T>
	struct $move_ctor<
		T,
		enable_if_t<
			std::is_move_constructible<T>::value && !std::is_const<T>::value &&
			!$is_initializer_list<T>::value && !std::is_reference<T>::value>> {
		static void value(void *ptr, void *src) {
			construct(
				*reinterpret_cast<remove_cv_t<T> *>(ptr),
				std::move(*reinterpret_cast<T *>(src)));
		}
	};

	template <typename T, typename = void>
	struct $move_new {
		static constexpr std::nullptr_t value = nullptr;
	};

	template <typename T>
	struct $move_new<
		T,
		enable_if_t<
			std::is_move_constructible<T>::value && !std::is_const<T>::value &&
			!$is_initializer_list<T>::value && !std::is_reference<T>::value>> {
		static void *value(void *src) {
			return new remove_cv_t<T>(std::move(*reinterpret_cast<T *>(src)));
		}
	};

	template <typename T, typename = void>
	struct $to_bool {
		static constexpr std::nullptr_t value = nullptr;
	};

	template <typename T>
	struct $to_bool<T, enable_if_t<std::is_convertible<T, bool>::value>> {
		static bool value(void *ptr) {
			return static_cast<bool>(*reinterpret_cast<T *>(ptr));
		}
	};

	template <typename T, typename = void>
	struct $eq {
		static constexpr std::nullptr_t value = nullptr;
	};

	template <typename T>
	struct $eq<
		T,
		void_t<decltype(std::declval<const T>() == std::declval<const T>())>> {
		static bool value(const void *a, const void *b) {
			return static_cast<bool>(
				*reinterpret_cast<const T *>(a) ==
				*reinterpret_cast<const T *>(b));
		}
	};

#ifdef RUA_HAS_RTTI
	template <typename T>
	static const std::type_info &$std_id() {
		return typeid(T);
	}
#endif

	template <typename T>
	static const $table_t &$table() RUA_NOEXCEPT {
		RUA_SASSERT(!std::is_same<T, void>::value);

		static const $table_t tab{
			sizeof(T),
			alignof(T),
			std::is_trivial<T>::value,
			type_name<T>,
			$dtor<T>::value,
			$del<T>::value,
			$copy_ctor<T>::value,
			$copy_new<T>::value,
			$move_ctor<T>::value,
			$move_new<T>::value,
			$to_bool<T>::value,
			$eq<T>::value
#ifdef RUA_HAS_RTTI
			,
			&$std_id<T>
#endif
		};
		return tab;
	}

	const $table_t *$tab;
};

template <typename T, typename TypeInfo = type_info>
inline constexpr TypeInfo type_id() {
	return TypeInfo(in_place_type_t<T>{});
}

#ifdef RUA_HAS_RTTI

inline bool operator==(const type_info &a, std::type_index b) {
	return b == a.std_id();
}

inline bool operator!=(const type_info &a, std::type_index b) {
	return b != a.std_id();
}

inline bool operator==(std::type_index a, const type_info &b) {
	return a == b.std_id();
}

inline bool operator!=(std::type_index a, const type_info &b) {
	return a != b.std_id();
}

#endif

template <typename TypeInfo = type_info>
class typed_base {
public:
	const type_info &type() const {
		return $ti;
	}

	template <typename T>
	bool type_is() const {
		return $ti == type_id<T>();
	}

protected:
	constexpr typed_base() = default;

	constexpr explicit typed_base(TypeInfo ti) : $ti(std::move(ti)) {}

	template <typename T>
	constexpr explicit typed_base(in_place_type_t<T> ipt) : $ti(ipt) {}

	TypeInfo &$type() {
		return $ti;
	}

	const TypeInfo &$type() const {
		return $ti;
	}

	template <typename T = void>
	void $type_reset() {
		$ti = TypeInfo(in_place_type_t<T>{});
	}

	void $type_reset(TypeInfo ti) {
		$ti = std::move(ti);
	}

private:
	TypeInfo $ti;
};

} // namespace rua

#include <functional>

namespace std {

template <>
class hash<rua::type_info> {
public:
	size_t operator()(const rua::type_info &ti) const {
		return ti.hash_code();
	}
};

} // namespace std

#endif
