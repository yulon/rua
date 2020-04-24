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

template <typename T, typename = void>
struct type_name {
#ifdef RUA_RTTI
	static string_view get() {
		return typeid(T).name();
	}
#else
	static constexpr string_view get() {
		return "unknown_type";
	}
#endif
};

template <>
struct type_name<void> {
	static constexpr string_view get() {
		return "void";
	}
};

template <>
struct type_name<std::nullptr_t> {
	static constexpr string_view get() {
		return "nullptr_t";
	}
};

template <>
struct type_name<int> {
	static constexpr string_view get() {
		return "int";
	}
};

template <>
struct type_name<size_t> {
	static constexpr string_view get() {
		return "size_t";
	}
};

template <typename T>
struct type_name<
	T,
	enable_if_t<!std::is_const<T>::value && std::is_same<T, uint>::value>> {
	static constexpr string_view get() {
		return "uint";
	}
};

template <>
struct type_name<char> {
	static constexpr string_view get() {
		return "char";
	}
};

template <typename T>
struct type_name<
	T,
	enable_if_t<!std::is_const<T>::value && std::is_same<T, schar>::value>> {
	static constexpr string_view get() {
		return "schar";
	}
};

template <typename T>
struct type_name<
	T,
	enable_if_t<!std::is_const<T>::value && std::is_same<T, uchar>::value>> {
	static constexpr string_view get() {
		return "uchar";
	}
};

template <>
struct type_name<bool> {
	static constexpr string_view get() {
		return "bool";
	}
};

template <typename T>
struct type_name<
	T,
	enable_if_t<
		!std::is_const<T>::value && std::is_signed<T>::value &&
		!std::is_same<T, schar>::value &&
		!std::is_same<T, max_align_t>::value && !std::is_volatile<T>::value>> {
	static string_view get() {
		static const auto n =
			str_join("int", std::to_string(sizeof(T) * 8), "_t");
		return n;
	}
};

template <typename T>
struct type_name<
	T,
	enable_if_t<
		!std::is_const<T>::value && std::is_unsigned<T>::value &&
		!std::is_same<T, uint>::value && !std::is_same<T, uchar>::value &&
		!std::is_same<T, max_align_t>::value && !std::is_volatile<T>::value>> {
	static string_view get() {
		static const auto n =
			str_join("uint", std::to_string(sizeof(T) * 8), "_t");
		return n;
	}
};

template <>
struct type_name<float> {
	static constexpr string_view get() {
		return "float";
	}
};

template <>
struct type_name<double> {
	static constexpr string_view get() {
		return "double";
	}
};

template <typename T>
struct type_name<
	T,
	enable_if_t<
		!std::is_const<T>::value && std::is_same<T, max_align_t>::value>> {
	static constexpr string_view get() {
		return "max_align_t";
	}
};

#ifdef __cpp_lib_byte
template <>
struct type_name<std::byte> {
	static constexpr string_view get() {
		return "std::byte";
	}
};
#endif

template <typename T>
struct type_name<const T, enable_if_t<!std::is_pointer<T>::value>> {
	static string_view get() {
		static const auto n = str_join("const ", type_name<T>::get());
		return n;
	}
};

template <typename T>
struct type_name<T *> {
	static string_view get() {
		static const auto n = str_join(type_name<T>::get(), " *");
		return n;
	}
};

template <typename T>
struct type_name<T *&> {
	static string_view get() {
		static const auto n = str_join(type_name<T>::get(), " *&");
		return n;
	}
};

template <typename T>
struct type_name<T *&&> {
	static string_view get() {
		static const auto n = str_join(type_name<T>::get(), " *&&");
		return n;
	}
};

template <typename T>
struct type_name<T *const> {
	static string_view get() {
		static const auto n = str_join(type_name<T>::get(), " *const");
		return n;
	}
};

template <typename T>
struct type_name<T *const &> {
	static string_view get() {
		static const auto n = str_join(type_name<T>::get(), " *const &");
		return n;
	}
};

template <typename T>
struct type_name<T &> {
	static string_view get() {
		static const auto n = str_join(type_name<T>::get(), " &");
		return n;
	}
};

template <typename T>
struct type_name<T &&> {
	static string_view get() {
		static const auto n = str_join(type_name<T>::get(), " &&");
		return n;
	}
};

template <typename T, size_t N>
struct type_name<T[N]> {
	static string_view get() {
		static const auto n =
			str_join(type_name<T>::get(), "[", std::to_string(N), "]");
		return n;
	}
};

template <typename T, size_t N>
struct type_name<T (&)[N]> {
	static string_view get() {
		static const auto n =
			str_join(type_name<T>::get(), " (&)[", std::to_string(N), "]");
		return n;
	}
};

template <typename T, size_t N>
struct type_name<T(&&)[N]> {
	static string_view get() {
		static const auto n =
			str_join(type_name<T>::get(), " (&&)[", std::to_string(N), "]");
		return n;
	}
};

template <typename T, size_t N>
struct type_name<T (*)[N]> {
	static string_view get() {
		static const auto n =
			str_join(type_name<T>::get(), " (*)[", std::to_string(N), "]");
		return n;
	}
};

template <typename T, size_t N>
struct type_name<T (*&)[N]> {
	static string_view get() {
		static const auto n =
			str_join(type_name<T>::get(), " (*&)[", std::to_string(N), "]");
		return n;
	}
};

template <typename T, size_t N>
struct type_name<T (*&&)[N]> {
	static string_view get() {
		static const auto n =
			str_join(type_name<T>::get(), " (*&&)[", std::to_string(N), "]");
		return n;
	}
};

template <typename T, size_t N>
struct type_name<T (*const)[N]> {
	static string_view get() {
		static const auto n =
			str_join(type_name<T>::get(), " (*const)[", std::to_string(N), "]");
		return n;
	}
};

template <typename T, size_t N>
struct type_name<T (*const &)[N]> {
	static string_view get() {
		static const auto n = str_join(
			type_name<T>::get(), " (*const &)[", std::to_string(N), "]");
		return n;
	}
};

template <typename R, typename... Args>
struct type_name<R(Args...)> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			"(",
			str_join(type_name<Args>::get()..., ", "),
			")");
		return n;
	}
};

template <typename R, typename... Args>
struct type_name<R (*)(Args...)> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (*)(",
			str_join({type_name<Args>::get()...}, ", "),
			")");
		return n;
	}
};

template <typename R, typename... Args>
struct type_name<R (*&)(Args...)> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (*&)(",
			str_join({type_name<Args>::get()...}, ", "),
			")");
		return n;
	}
};

template <typename R, typename... Args>
struct type_name<R (*&&)(Args...)> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (*&&)(",
			str_join({type_name<Args>::get()...}, ", "),
			")");
		return n;
	}
};

template <typename R, typename... Args>
struct type_name<R (*const)(Args...)> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (*const)(",
			str_join({type_name<Args>::get()...}, ", "),
			")");
		return n;
	}
};

template <typename R, typename... Args>
struct type_name<R (*const &)(Args...)> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (*const &)(",
			str_join({type_name<Args>::get()...}, ", "),
			")");
		return n;
	}
};

template <typename R, typename... Args>
struct type_name<R (&)(Args...)> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (&)(",
			str_join({type_name<Args>::get()...}, ", "),
			")");
		return n;
	}
};

template <typename R, typename... Args>
struct type_name<R(&&)(Args...)> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (&&)(",
			str_join({type_name<Args>::get()...}, ", "),
			")");
		return n;
	}
};

template <typename T, typename E>
struct type_name<E T::*> {
	static string_view get() {
		static const auto n =
			str_join(type_name<E>::get(), " ", type_name<T>::get(), "::*");
		return n;
	}
};

template <typename T, typename E>
struct type_name<E T::*&> {
	static string_view get() {
		static const auto n =
			str_join(type_name<E>::get(), " ", type_name<T>::get(), "::*&");
		return n;
	}
};

template <typename T, typename E>
struct type_name<E T::*&&> {
	static string_view get() {
		static const auto n =
			str_join(type_name<E>::get(), " ", type_name<T>::get(), "::*&&");
		return n;
	}
};

template <typename T, typename E>
struct type_name<E T::*const> {
	static string_view get() {
		static const auto n =
			str_join(type_name<E>::get(), " ", type_name<T>::get(), "::*const");
		return n;
	}
};

template <typename T, typename E>
struct type_name<E T::*const &> {
	static string_view get() {
		static const auto n = str_join(
			type_name<E>::get(), " ", type_name<T>::get(), "::*const &");
		return n;
	}
};

template <typename T, typename R, typename... Args>
struct type_name<R (T::*)(Args...)> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (",
			type_name<T>::get(),
			"::*)(",
			str_join({type_name<Args>::get()...}, ", "),
			")");
		return n;
	}
};

template <typename T, typename R, typename... Args>
struct type_name<R (T::*&)(Args...)> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (",
			type_name<T>::get(),
			"::*&)(",
			str_join({type_name<Args>::get()...}, ", "),
			")");
		return n;
	}
};

template <typename T, typename R, typename... Args>
struct type_name<R (T::*&&)(Args...)> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (",
			type_name<T>::get(),
			"::*&&)(",
			str_join({type_name<Args>::get()...}, ", "),
			")");
		return n;
	}
};

template <typename T, typename R, typename... Args>
struct type_name<R (T::*const)(Args...)> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (",
			type_name<T>::get(),
			"::*const)(",
			str_join({type_name<Args>::get()...}, ", "),
			")");
		return n;
	}
};

template <typename T, typename R, typename... Args>
struct type_name<R (T::*const &)(Args...)> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (",
			type_name<T>::get(),
			"::*const &)(",
			str_join({type_name<Args>::get()...}, ", "),
			")");
		return n;
	}
};

template <typename T, typename R, typename... Args>
struct type_name<R (T::*)(Args...) const> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (",
			type_name<T>::get(),
			"::*)(",
			str_join({type_name<Args>::get()...}, ", "),
			") const");
		return n;
	}
};

template <typename T, typename R, typename... Args>
struct type_name<R (T::*&)(Args...) const> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (",
			type_name<T>::get(),
			"::*&)(",
			str_join({type_name<Args>::get()...}, ", "),
			") const");
		return n;
	}
};

template <typename T, typename R, typename... Args>
struct type_name<R (T::*&&)(Args...) const> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (",
			type_name<T>::get(),
			"::*&&)(",
			str_join({type_name<Args>::get()...}, ", "),
			") const");
		return n;
	}
};

template <typename T, typename R, typename... Args>
struct type_name<R (T::*const)(Args...) const> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (",
			type_name<T>::get(),
			"::*const)(",
			str_join({type_name<Args>::get()...}, ", "),
			") const");
		return n;
	}
};

template <typename T, typename R, typename... Args>
struct type_name<R (T::*const &)(Args...) const> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (",
			type_name<T>::get(),
			"::*const &)(",
			str_join({type_name<Args>::get()...}, ", "),
			") const");
		return n;
	}
};

template <typename T, typename R, typename... Args>
struct type_name<R (T::*)(Args...) const &> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (",
			type_name<T>::get(),
			"::*)(",
			str_join({type_name<Args>::get()...}, ", "),
			") const &");
		return n;
	}
};

template <typename T, typename R, typename... Args>
struct type_name<R (T::*&)(Args...) const &> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (",
			type_name<T>::get(),
			"::*&)(",
			str_join({type_name<Args>::get()...}, ", "),
			") const &");
		return n;
	}
};

template <typename T, typename R, typename... Args>
struct type_name<R (T::*&&)(Args...) const &> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (",
			type_name<T>::get(),
			"::*&&)(",
			str_join({type_name<Args>::get()...}, ", "),
			") const &");
		return n;
	}
};

template <typename T, typename R, typename... Args>
struct type_name<R (T::*const)(Args...) const &> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (",
			type_name<T>::get(),
			"::*const)(",
			str_join({type_name<Args>::get()...}, ", "),
			") const &");
		return n;
	}
};

template <typename T, typename R, typename... Args>
struct type_name<R (T::*const &)(Args...) const &> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (",
			type_name<T>::get(),
			"::*const &)(",
			str_join({type_name<Args>::get()...}, ", "),
			") const &");
		return n;
	}
};

template <typename T, typename R, typename... Args>
struct type_name<R (T::*)(Args...) &&> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (",
			type_name<T>::get(),
			"::*)(",
			str_join({type_name<Args>::get()...}, ", "),
			") &&");
		return n;
	}
};

template <typename T, typename R, typename... Args>
struct type_name<R (T::*&)(Args...) &&> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (",
			type_name<T>::get(),
			"::*&)(",
			str_join({type_name<Args>::get()...}, ", "),
			") &&");
		return n;
	}
};

template <typename T, typename R, typename... Args>
struct type_name<R (T::*&&)(Args...) &&> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (",
			type_name<T>::get(),
			"::*&&)(",
			str_join({type_name<Args>::get()...}, ", "),
			") &&");
		return n;
	}
};

template <typename T, typename R, typename... Args>
struct type_name<R (T::*const)(Args...) &&> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (",
			type_name<T>::get(),
			"::*const)(",
			str_join({type_name<Args>::get()...}, ", "),
			") &&");
		return n;
	}
};

template <typename T, typename R, typename... Args>
struct type_name<R (T::*const &)(Args...) &&> {
	static string_view get() {
		static const auto n = str_join(
			type_name<R>::get(),
			" (",
			type_name<T>::get(),
			"::*const &)(",
			str_join({type_name<Args>::get()...}, ", "),
			") &&");
		return n;
	}
};

template <>
struct type_name<std::string> {
	static string_view get() {
		return "std::string";
	}
};

template <>
struct type_name<std::wstring> {
	static string_view get() {
		return "std::wstring";
	}
};

template <typename CharT, typename Traits, typename Allocator>
struct type_name<std::basic_string<CharT, Traits, Allocator>> {
	static string_view get() {
		static const auto n = str_join(
			"std::basic_string<",
			type_name<CharT>::get(),
			", ",
			type_name<Traits>::get(),
			", ",
			type_name<Allocator>::get(),
			">");
		return n;
	}
};

template <>
struct type_name<string_view> {
	static string_view get() {
		return
#ifdef __cpp_lib_string_view
			"std::string_view"
#else
			"rua::string_view"
#endif
			;
	}
};

template <>
struct type_name<wstring_view> {
	static string_view get() {
		return
#ifdef __cpp_lib_string_view
			"std::wstring_view"
#else
			"rua::wstring_view"
#endif
			;
	}
};

template <typename CharT, typename Traits>
struct type_name<basic_string_view<CharT, Traits>> {
	static string_view get() {
		static const auto n = str_join(
#ifdef __cpp_lib_string_view
			"std::basic_string_view<"
#else
			"rua::basic_string_view<"
#endif
			,
			type_name<CharT>::get(),
			", ",
			type_name<Traits>::get(),
			">");
		return n;
	}
};

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

	RUA_FORCE_INLINE string_view name() const {
		return _tab ? _tab->name() : type_name<void>::get();
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

		string_view (*const name)();

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
			!_is_initializer_list<T>::value && !std::is_reference<T>::value>> {
		static void *value(void *src) {
			return new remove_cv_t<T>(std::move(*reinterpret_cast<T *>(src)));
		}
	};

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
								  type_name<T>::get,
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

template <typename T>
RUA_FORCE_INLINE constexpr type_info type_id() {
	return type_info(type_info::_table<T>());
}

template <>
RUA_FORCE_INLINE constexpr type_info type_id<void>() {
	return type_info();
}

template <typename T>
struct type_name<
	T,
	enable_if_t<!std::is_const<T>::value && std::is_class<T>::value>> {
	static string_view get() {
#ifdef RUA_RTTI
		return typeid(T).name();
#else
		static const auto n =
			str_join("class _", std::to_string(type_id<T>().hash_code()));
		return n;
#endif
	}
};

template <typename T>
struct type_name<
	T,
	enable_if_t<!std::is_const<T>::value && std::is_enum<T>::value>> {
	static string_view get() {
#ifdef RUA_RTTI
		return typeid(T).name();
#else
		static const auto n =
			str_join("enum _", std::to_string(type_id<T>().hash_code()));
		return n;
#endif
	}
};

template <typename T>
struct type_name<
	T,
	enable_if_t<!std::is_const<T>::value && std::is_union<T>::value>> {
	static string_view get() {
#ifdef RUA_RTTI
		return typeid(T).name();
#else
		static const auto n =
			str_join("union _", std::to_string(type_id<T>().hash_code()));
		return n;
#endif
	}
};

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
