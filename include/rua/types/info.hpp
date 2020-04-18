#ifndef _RUA_TYPES_INFO_HPP
#define _RUA_TYPES_INFO_HPP

#include "traits.hpp"
#include "util.hpp"

#include "../macros.hpp"
#include "../string/str_join.hpp"
#include "../string/string_view.hpp"

#include <cassert>
#include <string>

#ifdef RUA_RTTI
#include <typeindex>
#endif

namespace rua {

class type_info {
public:
	constexpr type_info() : _tab(nullptr) {}

	operator bool() const {
		return _tab;
	}

	bool operator==(const type_info &target) const {
		return _tab == target._tab;
	}

	bool operator!=(const type_info &target) const {
		return _tab != target._tab;
	}

	RUA_FORCE_INLINE size_t size() const {
		return _tab ? _tab->size : 0;
	}

	RUA_FORCE_INLINE size_t align() const {
		return _tab ? _tab->size : 0;
	}

	RUA_FORCE_INLINE bool is_trivial() const {
		return _tab ? _tab->is_trivial : false;
	}

	RUA_FORCE_INLINE const std::string &name() const {
		return _tab ? _tab->name() : _void_name();
	}

	RUA_FORCE_INLINE std::string &name() {
		return _tab ? _tab->name() : _void_name();
	}

	RUA_FORCE_INLINE size_t hash_code() const {
		return static_cast<size_t>(reinterpret_cast<uintptr_t>(_tab));
	}

	RUA_FORCE_INLINE void dtor(void *ptr) const {
		assert(_tab);
		if (!_tab->dtor) {
			return;
		}
		_tab->dtor(ptr);
	}

	RUA_FORCE_INLINE void del(void *ptr) const {
		assert(_tab);
		assert(_tab->del);
		_tab->del(ptr);
	}

	RUA_FORCE_INLINE bool is_copyable() const {
		return _tab && _tab->copy_ctor;
	}

	RUA_FORCE_INLINE void copy_ctor(void *ptr, const void *src) const {
		assert(_tab);
		assert(_tab->copy_ctor);
		_tab->copy_ctor(ptr, src);
	}

	RUA_FORCE_INLINE void *copy_new(const void *src) const {
		assert(_tab);
		assert(_tab->copy_new);
		return _tab->copy_new(src);
	}

	RUA_FORCE_INLINE bool is_moveable() const {
		return _tab && _tab->move_ctor;
	}

	RUA_FORCE_INLINE void move_ctor(void *ptr, void *src) const {
		assert(_tab);
		assert(_tab->move_ctor);
		_tab->move_ctor(ptr, src);
	}

	RUA_FORCE_INLINE void *move_new(void *src) const {
		assert(_tab);
		assert(_tab->move_new);
		return _tab->move_new(src);
	}

#ifdef RUA_RTTI
	RUA_FORCE_INLINE const std::type_info &std_id() const {
		return _tab ? _tab->std_id() : typeid(void);
	}
#endif

private:
	struct _table_t {
		const size_t size;

		const size_t align;

		const bool is_trivial;

		std::string &(*const name)();

		void (*const dtor)(void *);

		void (*const del)(void *);

		void (*const copy_ctor)(void *ptr, const void *src);

		void *(*const copy_new)(const void *src);

		void (*const move_ctor)(void *ptr, void *src);

		void *(*const move_new)(void *src);

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

	template <typename T, typename = void>
	struct _copy_ctor {
		static constexpr std::nullptr_t value = nullptr;
	};

	template <typename T>
	struct _copy_ctor<
		T,
		enable_if_t<
			std::is_copy_constructible<T>::value &&
			!std::is_reference<T>::value>> {
		static void value(void *ptr, const void *src) {
			new (reinterpret_cast<remove_cv_t<T> *>(ptr))
				T(*reinterpret_cast<const T *>(src));
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
			!std::is_reference<T>::value>> {
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
			std::is_move_constructible<T>::value &&
			!std::is_reference<T>::value>> {
		static void value(void *ptr, void *src) {
			new (reinterpret_cast<remove_cv_t<T> *>(ptr))
				T(std::move(*reinterpret_cast<T *>(src)));
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
			!std::is_reference<T>::value>> {
		static void *value(void *src) {
			return new remove_cv_t<T>(std::move(*reinterpret_cast<T *>(src)));
		}
	};

	template <typename T, typename = void>
	struct _name {
		static std::string &value() {
			static std::string n("unknown_type");
			return n;
		}
	};

	static std::string &_void_name() {
		static std::string n("void");
		return n;
	}

#ifdef RUA_RTTI
	template <typename T>
	static const std::type_info &_std_id() {
		return typeid(T);
	}
#endif

	template <typename T>
	static RUA_FORCE_INLINE const _table_t &_table() {
		RUA_SASSERT((!std::is_same<T, void>::value));

		static const _table_t tab{size_of<T>::value,
								  align_of<T>::value,
								  std::is_trivial<T>::value,
								  _name<T>::value,
								  _dtor<T>::value,
								  _del<T>::value,
								  _copy_ctor<T>::value,
								  _copy_new<T>::value,
								  _move_ctor<T>::value,
								  _move_new<T>::value
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
	friend RUA_FORCE_INLINE constexpr type_info type_id();
};

template <>
struct type_info::_name<void, void> {
	static std::string &value() {
		return _void_name();
	}
};

template <>
struct type_info::_name<int, void> {
	static std::string &value() {
		static std::string n("int");
		return n;
	}
};

template <>
struct type_info::_name<uint, void> {
	static std::string &value() {
		static std::string n("uint");
		return n;
	}
};

template <>
struct type_info::_name<char, void> {
	static std::string &value() {
		static std::string n("char");
		return n;
	}
};

template <>
struct type_info::_name<
	schar,
	enable_if_t<
		!std::is_same<schar, char>::value &&
		!std::is_same<schar, int>::value>> {
	static std::string &value() {
		static std::string n("schar");
		return n;
	}
};

template <>
struct type_info::_name<
	uchar,
	enable_if_t<
		!std::is_same<uchar, char>::value &&
		!std::is_same<uchar, uint>::value>> {
	static std::string &value() {
		static std::string n("uchar");
		return n;
	}
};

template <>
struct type_info::_name<bool, void> {
	static std::string &value() {
		static std::string n("bool");
		return n;
	}
};

template <typename T>
struct type_info::_name<
	T,
	enable_if_t<
		std::is_signed<T>::value && !std::is_const<T>::value &&
		!std::is_volatile<T>::value>> {
	static std::string &value() {
		static auto n = str_join("int", std::to_string(sizeof(T) * 8), "_t");
		return n;
	}
};

template <typename T>
struct type_info::_name<
	T,
	enable_if_t<
		std::is_unsigned<T>::value && !std::is_const<T>::value &&
		!std::is_volatile<T>::value>> {
	static std::string &value() {
		static auto n = str_join("uint", std::to_string(sizeof(T) * 8), "_t");
		return n;
	}
};

template <typename T>
struct type_info::_name<T const, enable_if_t<!std::is_pointer<T>::value>> {
	static std::string &value() {
		static auto n = str_join("const ", _name<T>::value());
		return n;
	}
};

template <typename T>
struct type_info::_name<T *, void> {
	static std::string &value() {
		static auto n = str_join(_name<T>::value(), " *");
		return n;
	}
};

template <typename T>
struct type_info::_name<T *const, void> {
	static std::string &value() {
		static auto n = str_join(_name<T>::value(), " *const");
		return n;
	}
};

template <typename T>
struct type_info::_name<T &, void> {
	static std::string &value() {
		static auto n = str_join(_name<T>::value(), " &");
		return n;
	}
};

template <typename T>
struct type_info::_name<T &&, void> {
	static std::string &value() {
		static auto n = str_join(_name<T>::value(), " &&");
		return n;
	}
};

template <typename R, typename... Args>
struct type_info::_name<R(Args...), void> {
	static std::string &value() {
		static auto n = str_join(_name<R>::value(), "(...)");
		return n;
	}
};

template <typename R, typename... Args>
struct type_info::_name<R (*)(Args...), void> {
	static std::string &value() {
		static auto n = str_join(_name<R>::value(), " (*)(...)");
		return n;
	}
};

template <typename R, typename... Args>
struct type_info::_name<R (&)(Args...), void> {
	static std::string &value() {
		static auto n = str_join(_name<R>::value(), " (&)(...)");
		return n;
	}
};

template <typename R, typename... Args>
struct type_info::_name<R (*const)(Args...), void> {
	static std::string &value() {
		static auto n = str_join(_name<R>::value(), " (*const)(...)");
		return n;
	}
};

template <typename T, size_t N>
struct type_info::_name<T[N], void> {
	static std::string &value() {
		static auto n =
			str_join(_name<T>::value(), "[", std::to_string(N), "]");
		return n;
	}
};

template <typename T, size_t N>
struct type_info::_name<T (&)[N], void> {
	static std::string &value() {
		static auto n =
			str_join(_name<T>::value(), " (&)[", std::to_string(N), "]");
		return n;
	}
};

template <typename T, size_t N>
struct type_info::_name<T(&&)[N], void> {
	static std::string &value() {
		static auto n =
			str_join(_name<T>::value(), " (&&)[", std::to_string(N), "]");
		return n;
	}
};

template <typename T, size_t N>
struct type_info::_name<T (*)[N], void> {
	static std::string &value() {
		static auto n =
			str_join(_name<T>::value(), " (*)[", std::to_string(N), "]");
		return n;
	}
};

template <typename T, size_t N>
struct type_info::_name<T (*const)[N], void> {
	static std::string &value() {
		static auto n =
			str_join(_name<T>::value(), " (*const)[", std::to_string(N), "]");
		return n;
	}
};

template <>
struct type_info::_name<std::string, void> {
	static std::string &value() {
		static std::string n("std::string");
		return n;
	}
};

template <>
struct type_info::_name<std::wstring, void> {
	static std::string &value() {
		static std::string n("std::wstring");
		return n;
	}
};

template <typename CharT, typename Traits, typename Allocator>
struct type_info::_name<std::basic_string<CharT, Traits, Allocator>, void> {
	static std::string &value() {
		static auto n = str_join(
			"std::basic_string<",
			_name<CharT>::value(),
			", ",
			_name<Traits>::value(),
			", ",
			_name<Allocator>::value(),
			">");
		return n;
	}
};

template <>
struct type_info::_name<string_view, void> {
	static std::string &value() {
		static std::string n(
#ifdef __cpp_lib_string_view
			"std::string_view"
#else
			"rua::string_view"
#endif
		);
		return n;
	}
};

template <>
struct type_info::_name<wstring_view, void> {
	static std::string &value() {
		static std::string n(
#ifdef __cpp_lib_string_view
			"std::wstring_view"
#else
			"rua::wstring_view"
#endif
		);
		return n;
	}
};

template <typename CharT, typename Traits>
struct type_info::_name<basic_string_view<CharT, Traits>, void> {
	static std::string &value() {
		static auto n = str_join(
#ifdef __cpp_lib_string_view
			"std::basic_string_view<"
#else
			"rua::basic_string_view<"
#endif
			,
			_name<CharT>::value(),
			", ",
			_name<Traits>::value(),
			">");
		return n;
	}
};

template <typename T>
RUA_FORCE_INLINE constexpr type_info type_id() {
	return type_info(type_info::_table<T>());
}

template <>
RUA_FORCE_INLINE constexpr type_info type_id<void>() {
	return type_info();
}

#ifdef RUA_RTTI

RUA_FORCE_INLINE bool operator==(const type_info &a, std::type_index b) {
	return b == a.std_id();
}

RUA_FORCE_INLINE bool operator!=(const type_info &a, std::type_index b) {
	return b != a.std_id();
}

RUA_FORCE_INLINE bool operator==(std::type_index a, const type_info &b) {
	return a == b.std_id();
}

RUA_FORCE_INLINE bool operator!=(std::type_index a, const type_info &b) {
	return a != b.std_id();
}

#endif

class enable_type_info {
public:
	RUA_FORCE_INLINE type_info type() const {
		return _type;
	}

	template <typename T>
	RUA_FORCE_INLINE bool type_is() const {
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
