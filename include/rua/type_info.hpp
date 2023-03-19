#ifndef _rua_type_info_hpp
#define _rua_type_info_hpp

#include "string/join.hpp"
#include "string/len.hpp"
#include "string/view.hpp"
#include "util.hpp"

#include <cassert>
#include <string>

#ifdef RUA_HAS_RTTI
#include <typeindex>
#endif

namespace rua {

template <typename T>
inline constexpr const char *_func_name() {
#if defined(__GNUC__) || defined(__clang__)
	return __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
	return __FUNCSIG__;
#endif
}

template <typename T>
inline constexpr size_t _func_name_prefix_len() {
#if defined(__GNUC__) || defined(__clang__)
	return c_str_len("constexpr const char* rua::_func_name() [with T = ");
#elif defined(_MSC_VER)
	return c_str_len("const char *__cdecl rua::_func_name<");
#endif
}

template <typename T>
inline constexpr size_t _func_name_suffix_len() {
#if defined(__GNUC__) || defined(__clang__)
	return c_str_len("]");
#elif defined(_MSC_VER)
	return c_str_len(">(void)");
#endif
}

template <typename T>
inline constexpr string_view _type_name(size_t func_name_prefix_len) {
	return string_view(
		_func_name<T>() + func_name_prefix_len,
		c_str_len(_func_name<T>()) -
			(func_name_prefix_len + _func_name_suffix_len<T>()));
}

template <typename T>
inline constexpr string_view type_name() {
	return _type_name<T>(_func_name_prefix_len<T>());
}

class type_info {
public:
	constexpr type_info(std::nullptr_t = nullptr) : $tab(nullptr) {}

	operator bool() const {
		return $tab;
	}

	bool operator==(const type_info &target) const {
		return $tab == target.$tab;
	}

	bool operator!=(const type_info &target) const {
		return $tab != target.$tab;
	}

	size_t size() const {
		return $tab ? $tab->size : 0;
	}

	size_t align() const {
		return $tab ? $tab->size : 0;
	}

	bool is_trivial() const {
		return $tab ? $tab->is_trivial : false;
	}

	string_view name() const {
		return $tab ? $tab->name() : type_name<void>();
	}

	size_t hash_code() const {
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

	bool is_copyable() const {
		return $tab && $tab->copy_ctor;
	}

	void copy_to(void *ptr, const void *src) const {
		assert($tab);
		assert($tab->copy_ctor);
		$tab->copy_ctor(ptr, src);
	}

	void *copy_to_new(const void *src) const {
		assert($tab);
		assert($tab->copy_new);
		return $tab->copy_new(src);
	}

	bool is_moveable() const {
		return $tab && $tab->move_ctor;
	}

	void move_to(void *ptr, void *src) const {
		assert($tab);
		assert($tab->move_ctor);
		$tab->move_ctor(ptr, src);
	}

	void *move_to_new(void *src) const {
		assert($tab);
		assert($tab->move_new);
		return $tab->move_new(src);
	}

	bool is_convertable_to_bool() const {
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

	void reset() {
		$tab = nullptr;
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
	static const $table_t &$table() {
		RUA_SASSERT(!std::is_same<T, void>::value);

		static const $table_t tab{
			size_of<T>::value,
			align_of<T>::value,
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

	type_info(const $table_t &tab) : $tab(&tab) {}

	template <typename T>
	friend inline constexpr type_info type_id();
};

template <typename T>
inline constexpr type_info type_id() {
	return type_info(type_info::$table<T>());
}

template <>
inline constexpr type_info type_id<void>() {
	return type_info();
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

class enable_type_info {
public:
	type_info type() const {
		return $type;
	}

	template <typename T>
	bool type_is() const {
		return $type == type_id<T>();
	}

protected:
	constexpr enable_type_info() : $type() {}

	constexpr enable_type_info(type_info ti) : $type(ti) {}

	type_info $type;
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
