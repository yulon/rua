#ifndef _RUA_TYPES_INFO_HPP
#define _RUA_TYPES_INFO_HPP

#include "traits.hpp"
#include "util.hpp"

#include "../macros.hpp"
#include "../string/join.hpp"
#include "../string/len.hpp"
#include "../string/view.hpp"

#include <cassert>
#include <string>

#ifdef RUA_RTTI
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
	constexpr type_info(std::nullptr_t = nullptr) : _tab(nullptr) {}

	operator bool() const {
		return _tab;
	}

	bool operator==(const type_info &target) const {
		return _tab == target._tab;
	}

	bool operator!=(const type_info &target) const {
		return _tab != target._tab;
	}

	size_t size() const {
		return _tab ? _tab->size : 0;
	}

	size_t align() const {
		return _tab ? _tab->size : 0;
	}

	bool is_trivial() const {
		return _tab ? _tab->is_trivial : false;
	}

	string_view name() const {
		return _tab ? _tab->name() : type_name<void>();
	}

	size_t hash_code() const {
		return static_cast<size_t>(reinterpret_cast<uintptr_t>(_tab));
	}

	void destruct(void *ptr) const {
		assert(_tab);
		if (!_tab->dtor) {
			return;
		}
		_tab->dtor(ptr);
	}

	void dealloc(void *ptr) const {
		assert(_tab);
		assert(_tab->del);
		_tab->del(ptr);
	}

	bool is_copyable() const {
		return _tab && _tab->copy_ctor;
	}

	void copy_to(void *ptr, const void *src) const {
		assert(_tab);
		assert(_tab->copy_ctor);
		_tab->copy_ctor(ptr, src);
	}

	void *copy_to_new(const void *src) const {
		assert(_tab);
		assert(_tab->copy_new);
		return _tab->copy_new(src);
	}

	bool is_moveable() const {
		return _tab && _tab->move_ctor;
	}

	void move_to(void *ptr, void *src) const {
		assert(_tab);
		assert(_tab->move_ctor);
		_tab->move_ctor(ptr, src);
	}

	void *move_to_new(void *src) const {
		assert(_tab);
		assert(_tab->move_new);
		return _tab->move_new(src);
	}

	bool is_convertable_to_bool() const {
		return _tab && _tab->to_bool;
	}

	bool convert_to_bool(void *ptr) const {
		assert(_tab);
		assert(_tab->to_bool);
		return _tab->to_bool(ptr);
	}

#ifdef RUA_RTTI
	const std::type_info &std_id() const {
		return _tab ? _tab->std_id() : typeid(void);
	}
#endif

private:
	struct _table_t {
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

#ifdef RUA_RTTI
		const std::type_info &(*const std_id)();
#endif
	};

	template <typename T, typename = void>
	struct _dtor {
		static constexpr std::nullptr_t value = nullptr;
	};

	template <typename T>
	struct _dtor<
		T,
		enable_if_t<
			size_of<T>::value && !std::is_trivial<T>::value &&
			!std::is_reference<T>::value>> {
		static void value(void *ptr) {
			reinterpret_cast<remove_cv_t<T> *>(ptr)->~T();
		}
	};

	template <typename T, typename = void>
	struct _del {
		static constexpr std::nullptr_t value = nullptr;
	};

	template <typename T>
	struct _del<
		T,
		enable_if_t<(size_of<T>::value > 0) && !std::is_reference<T>::value>> {
		static void value(void *ptr) {
			delete reinterpret_cast<remove_cv_t<T> *>(ptr);
		}
	};

	template <typename T>
	struct _is_initializer_list : std::false_type {};

	template <typename E>
	struct _is_initializer_list<std::initializer_list<E>> : std::true_type {};

	template <typename T, typename = void>
	struct _copy_ctor {
		static constexpr std::nullptr_t value = nullptr;
	};

	template <typename T>
	struct _copy_ctor<
		T,
		enable_if_t<
			std::is_copy_constructible<T>::value &&
			!_is_initializer_list<T>::value && !std::is_reference<T>::value>> {
		static void value(void *ptr, const void *src) {
			construct(
				*reinterpret_cast<remove_cv_t<T> *>(ptr),
				*reinterpret_cast<const T *>(src));
		};
	};

	template <typename T, typename = void>
	struct _copy_new {
		static constexpr std::nullptr_t value = nullptr;
	};

	template <typename T>
	struct _copy_new<
		T,
		enable_if_t<
			std::is_copy_constructible<T>::value &&
			!_is_initializer_list<T>::value && !std::is_reference<T>::value>> {
		static void *value(const void *src) {
			return new remove_cv_t<T>(*reinterpret_cast<const T *>(src));
		}
	};

	template <typename T, typename = void>
	struct _move_ctor {
		static constexpr std::nullptr_t value = nullptr;
	};

	template <typename T>
	struct _move_ctor<
		T,
		enable_if_t<
			std::is_move_constructible<T>::value && !std::is_const<T>::value &&
			!_is_initializer_list<T>::value && !std::is_reference<T>::value>> {
		static void value(void *ptr, void *src) {
			construct(
				*reinterpret_cast<remove_cv_t<T> *>(ptr),
				std::move(*reinterpret_cast<T *>(src)));
		}
	};

	template <typename T, typename = void>
	struct _move_new {
		static constexpr std::nullptr_t value = nullptr;
	};

	template <typename T>
	struct _move_new<
		T,
		enable_if_t<
			std::is_move_constructible<T>::value && !std::is_const<T>::value &&
			!_is_initializer_list<T>::value && !std::is_reference<T>::value>> {
		static void *value(void *src) {
			return new remove_cv_t<T>(std::move(*reinterpret_cast<T *>(src)));
		}
	};

	template <typename T, typename = void>
	struct _to_bool {
		static constexpr std::nullptr_t value = nullptr;
	};

	template <typename T>
	struct _to_bool<T, enable_if_t<std::is_convertible<T, bool>::value>> {
		static bool value(void *ptr) {
			return static_cast<bool>(*reinterpret_cast<T *>(ptr));
		}
	};

#ifdef RUA_RTTI
	template <typename T>
	static const std::type_info &_std_id() {
		return typeid(T);
	}
#endif

	template <typename T>
	static const _table_t &_table() {
		RUA_SASSERT((!std::is_same<T, void>::value));

		static const _table_t tab{
			size_of<T>::value,
			align_of<T>::value,
			std::is_trivial<T>::value,
			type_name<T>,
			_dtor<T>::value,
			_del<T>::value,
			_copy_ctor<T>::value,
			_copy_new<T>::value,
			_move_ctor<T>::value,
			_move_new<T>::value,
			_to_bool<T>::value
#ifdef RUA_RTTI
			,
			&_std_id<T>
#endif
		};
		return tab;
	}

	const _table_t *_tab;

	type_info(const _table_t &tab) : _tab(&tab) {}

	template <typename T>
	friend inline constexpr type_info type_id();
};

template <typename T>
inline constexpr type_info type_id() {
	return type_info(type_info::_table<T>());
}

template <>
inline constexpr type_info type_id<void>() {
	return type_info();
}

#ifdef RUA_RTTI

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
		return _type;
	}

	template <typename T>
	bool type_is() const {
		return _type == type_id<T>();
	}

protected:
	constexpr enable_type_info() : _type() {}

	constexpr enable_type_info(type_info ti) : _type(ti) {}

	type_info _type;
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
