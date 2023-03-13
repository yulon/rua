#ifndef _rua_generic_word_hpp
#define _rua_generic_word_hpp

#include "binary/bits.hpp"
#include "string/conv.hpp"
#include "util.hpp"

#include <cassert>

namespace rua {

class generic_word {
public:
	template <typename T>
	struct is_dynamic_allocation {
		static constexpr auto value =
			(size_of<T>::value > sizeof(uintptr_t) ||
			 !std::is_trivial<T>::value);
	};

	////////////////////////////////////////////////////////////////////////

	generic_word() = default;

	constexpr generic_word(std::nullptr_t) : _val(0) {}

	template <typename T, typename = enable_if_t<std::is_integral<T>::value>>
	constexpr generic_word(T val) : _val(static_cast<uintptr_t>(val)) {}

	template <typename T>
	generic_word(T *ptr) : _val(reinterpret_cast<uintptr_t>(ptr)) {}

	template <
		typename T,
		typename = enable_if_t<std::is_member_function_pointer<T>::value>>
	generic_word(const T &src) :
		generic_word(reinterpret_cast<const void *>(src)) {}

	template <
		typename T,
		typename DecayT = decay_t<T>,
		typename = enable_if_t<
			!std::is_integral<DecayT>::value &&
			!std::is_pointer<DecayT>::value &&
			!is_null_pointer<DecayT>::value &&
			!is_dynamic_allocation<DecayT>::value &&
			!std::is_base_of<generic_word, DecayT>::value>>
	generic_word(const T &src) {
		bit_set<DecayT>(&_val, src);
	}

	template <
		typename T,
		typename DecayT = decay_t<T>,
		typename = enable_if_t<is_dynamic_allocation<DecayT>::value>>
	generic_word(T &&src) :
		_val(reinterpret_cast<uintptr_t>(new DecayT(std::forward<T>(src)))) {}

	template <
		typename T,
		typename... Args,
		typename = enable_if_t<is_dynamic_allocation<T>::value>>
	explicit generic_word(in_place_type_t<T>, Args &&...args) :
		_val(reinterpret_cast<uintptr_t>(new T(std::forward<Args>(args)...))) {}

	template <
		typename T,
		typename U,
		typename... Args,
		typename = enable_if_t<is_dynamic_allocation<T>::value>>
	explicit generic_word(
		in_place_type_t<T>, std::initializer_list<U> il, Args &&...args) :
		_val(reinterpret_cast<uintptr_t>(
			new T(il, std::forward<Args>(args)...))) {}

	template <
		typename T,
		typename... Args,
		typename = enable_if_t<!std::is_void<T>::value>,
		typename = enable_if_t<!is_dynamic_allocation<T>::value>>
	explicit generic_word(in_place_type_t<T>, Args &&...args) :
		_val(T(std::forward<Args>(args)...)) {}

	explicit constexpr generic_word(in_place_type_t<void>) : _val(0) {}

	constexpr operator bool() const {
		return _val;
	}

	constexpr bool operator!() const {
		return !_val;
	}

	RUA_CONSTEXPR_14 uintptr_t &value() {
		return _val;
	}

	constexpr uintptr_t value() const {
		return _val;
	}

	template <typename T>
	enable_if_t<std::is_integral<T>::value, T> as() const & {
		return static_cast<T>(_val);
	}

	template <typename T>
	enable_if_t<std::is_pointer<T>::value, T> as() const & {
		return reinterpret_cast<T>(_val);
	}

	template <typename T>
	enable_if_t<
		!std::is_integral<T>::value && !std::is_pointer<T>::value &&
			!is_dynamic_allocation<T>::value,
		T>
	as() const & {
		return bit_get<T>(&_val);
	}

	template <typename T>
	enable_if_t<is_dynamic_allocation<T>::value, const T &> as() const & {
		return *reinterpret_cast<const T *>(_val);
	}

	template <typename T>
	enable_if_t<is_dynamic_allocation<T>::value, T &> as() & {
		return *reinterpret_cast<T *>(_val);
	}

	template <typename T>
	enable_if_t<is_dynamic_allocation<T>::value, T> as() && {
		auto ptr = reinterpret_cast<T *>(_val);
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

	template <
		typename T,
		typename = enable_if_t<is_dynamic_allocation<T>::value>>
	void destruct() {
		delete reinterpret_cast<T *>(_val);
	}

private:
	uintptr_t _val;
};

inline std::string to_string(const generic_word w) {
	return to_string(w.value());
}

} // namespace rua

namespace std {

template <>
struct hash<rua::generic_word> {
	constexpr size_t operator()(const rua::generic_word w) const {
		return static_cast<size_t>(w.value());
	}
};

} // namespace std

#endif
