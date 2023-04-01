#ifndef _rua_any_word_hpp
#define _rua_any_word_hpp

#include "binary/bits.hpp"
#include "string/conv.hpp"
#include "util.hpp"

#include <cassert>

namespace rua {

class any_word {
public:
	template <typename T>
	struct is_placeable : bool_constant<
							  RUA_IS_PLACEABLE(
								  size_of<T>::value,
								  align_of<T>::value,
								  sizeof(uintptr_t),
								  alignof(uintptr_t)) &&
							  std::is_trivial<T>::value> {};

	////////////////////////////////////////////////////////////////////////

	any_word() = default;

	constexpr any_word(std::nullptr_t) : $val(0) {}

	template <typename T, typename = enable_if_t<std::is_integral<T>::value>>
	constexpr any_word(T val) : $val(static_cast<uintptr_t>(val)) {}

	template <typename T>
	any_word(T *ptr) : $val(reinterpret_cast<uintptr_t>(ptr)) {}

	template <
		typename T,
		typename = enable_if_t<std::is_member_function_pointer<T>::value>>
	any_word(const T &src) : any_word(reinterpret_cast<const void *>(src)) {}

	template <
		typename T,
		typename DecayT = decay_t<T>,
		typename = enable_if_t<
			!std::is_integral<DecayT>::value &&
			!std::is_pointer<DecayT>::value &&
			!is_null_pointer<DecayT>::value && is_placeable<DecayT>::value &&
			!std::is_base_of<any_word, DecayT>::value>>
	any_word(const T &src) {
		bit_set<DecayT>(&$val, src);
	}

	template <
		typename T,
		typename DecayT = decay_t<T>,
		typename = enable_if_t<!is_placeable<DecayT>::value>>
	explicit any_word(T &&src) :
		$val(reinterpret_cast<uintptr_t>(new DecayT(std::forward<T>(src)))) {}

	template <
		typename T,
		typename... Args,
		typename = enable_if_t<!is_placeable<T>::value>>
	explicit any_word(in_place_type_t<T>, Args &&...args) :
		$val(reinterpret_cast<uintptr_t>(new T(std::forward<Args>(args)...))) {}

	template <
		typename T,
		typename U,
		typename... Args,
		typename = enable_if_t<!is_placeable<T>::value>>
	explicit any_word(
		in_place_type_t<T>, std::initializer_list<U> il, Args &&...args) :
		$val(reinterpret_cast<uintptr_t>(
			new T(il, std::forward<Args>(args)...))) {}

	template <
		typename T,
		typename... Args,
		typename = enable_if_t<!std::is_void<T>::value>,
		typename = enable_if_t<is_placeable<T>::value>>
	explicit any_word(in_place_type_t<T>, Args &&...args) :
		$val(T(std::forward<Args>(args)...)) {}

	explicit constexpr any_word(in_place_type_t<void>) : $val(0) {}

	constexpr operator bool() const {
		return $val;
	}

	constexpr bool operator!() const {
		return !$val;
	}

	RUA_CONSTEXPR_14 uintptr_t &value() {
		return $val;
	}

	constexpr uintptr_t value() const {
		return $val;
	}

	template <typename T>
	enable_if_t<std::is_integral<T>::value, T> as() const & {
		return static_cast<T>($val);
	}

	template <typename T>
	enable_if_t<std::is_pointer<T>::value, T> as() const & {
		return reinterpret_cast<T>($val);
	}

	template <typename T>
	enable_if_t<
		!std::is_integral<T>::value && !std::is_pointer<T>::value &&
			is_placeable<T>::value,
		T>
	as() const & {
		return bit_get<T>(&$val);
	}

	template <typename T>
	enable_if_t<!is_placeable<T>::value, const T &> as() const & {
		return *reinterpret_cast<const T *>($val);
	}

	template <typename T>
	enable_if_t<!is_placeable<T>::value, T &> as() & {
		return *reinterpret_cast<T *>($val);
	}

	template <typename T>
	enable_if_t<!is_placeable<T>::value, T> as() && {
		auto ptr = reinterpret_cast<T *>($val);
		T r(std::move(*ptr));
		delete ptr;
		return r;
	}

	template <
		typename T,
		typename = enable_if_t<
			std::is_integral<T>::value || std::is_pointer<T>::value>>
	operator T() const {
		return as<T>();
	}

	template <typename T, typename = enable_if_t<!is_placeable<T>::value>>
	void destruct() {
		delete reinterpret_cast<T *>($val);
	}

private:
	uintptr_t $val;
};

inline std::string to_string(const any_word w) {
	return to_string(w.value());
}

} // namespace rua

namespace std {

template <>
struct hash<rua::any_word> {
	constexpr size_t operator()(const rua::any_word w) const {
		return static_cast<size_t>(w.value());
	}
};

} // namespace std

#endif
