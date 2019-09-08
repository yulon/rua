#ifndef _RUA_ANY_HPP
#define _RUA_ANY_HPP

#include "bit.hpp"
#include "type_traits/measures.hpp"
#include "type_traits/std_patch.hpp"
#include "type_traits/type_info.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <type_traits>
#include <utility>

namespace rua {

#define _RUA_ANY_IS_DYNAMIC_ALLOCATION(val_len, val_align, sto_len, sto_align) \
	(val_len > sto_len || val_align > sto_align)

template <size_t StorageLen, size_t StorageAlign>
class basic_any {
public:
	template <typename T>
	struct is_dynamic_allocation {
		static constexpr auto value = _RUA_ANY_IS_DYNAMIC_ALLOCATION(
			size_of<T>::value, align_of<T>::value, StorageLen, StorageAlign);
	};

	template <typename T>
	struct is_destructible {
		static constexpr auto value =
			is_dynamic_allocation<T>::value || !std::is_trivial<T>::value;
	};

	////////////////////////////////////////////////////////////////////////

	constexpr basic_any() : _sto(), _typ_inf(nullptr) {}

	template <
		typename T,
		typename = typename std::enable_if<
			!std::is_base_of<basic_any, typename std::decay<T>::type>::value>::
			type>
	basic_any(T &&val) {
		_emplace<typename std::decay<T>::type>(std::forward<T>(val));
	}

	template <typename T, typename... Args>
	basic_any(in_place_type_t<T> ipt, Args &&... args) {
		_emplace<T>(std::forward<Args>(args)...);
	}

	template <typename T, typename U, typename... Args>
	basic_any(
		in_place_type_t<T>, std::initializer_list<U> il, Args &&... args) {
		_emplace<T>(il, std::forward<Args>(args)...);
	}

	~basic_any() {
		reset();
	}

	basic_any(const basic_any &src) : _typ_inf(src._typ_inf) {
		if (!_typ_inf) {
			return;
		}

		assert(_typ_inf->copy_ctor);

		if (_RUA_ANY_IS_DYNAMIC_ALLOCATION(
				_typ_inf->size, _typ_inf->align, StorageLen, StorageAlign)) {
			assert(_typ_inf->new_copy);
			*reinterpret_cast<void **>(&_sto) =
				_typ_inf->new_copy(*reinterpret_cast<void **>(&src._sto));
			return;
		}
		_typ_inf->copy_ctor(&_sto, &src._sto);
	}

	basic_any(basic_any &&src) : _typ_inf(src._typ_inf) {
		if (!_typ_inf) {
			return;
		}

		assert(_typ_inf->move_ctor);

		if (_RUA_ANY_IS_DYNAMIC_ALLOCATION(
				_typ_inf->size, _typ_inf->align, StorageLen, StorageAlign)) {
			*reinterpret_cast<void **>(&_sto) =
				*reinterpret_cast<void **>(&src._sto);
			src._typ_inf = nullptr;
			return;
		}
		_typ_inf->move_ctor(&_sto, &src._sto);
	}

	RUA_OVERLOAD_ASSIGNMENT(basic_any)

	bool has_value() const {
		return !type_is<void>();
	}

	operator bool() const {
		return has_value();
	}

	type_id_t type() const {
		return _typ_inf ? _typ_inf->id : type_id<void>();
	}

	template <typename T>
	bool type_is() const {
		return type() == type_id<T>();
	}

	template <typename T>
	typename std::enable_if<
		!is_dynamic_allocation<typename std::decay<T>::type>::value,
		T &>::type
	as() & {
		assert(type_is<T>());
		return *reinterpret_cast<T *>(&_sto);
	}

	template <typename T>
	typename std::enable_if<
		is_dynamic_allocation<typename std::decay<T>::type>::value,
		T &>::type
	as() & {
		assert(type_is<T>());
		return **reinterpret_cast<T **>(&_sto);
	}

	template <typename T>
	T &&as() && {
		return std::move(as<T>());
	}

	template <typename T>
	typename std::enable_if<
		!is_dynamic_allocation<typename std::decay<T>::type>::value,
		const T &>::type
	as() const & {
		assert(type_is<T>());
		return *reinterpret_cast<const T *>(&_sto);
	}

	template <typename T>
	typename std::enable_if<
		is_dynamic_allocation<typename std::decay<T>::type>::value,
		const T &>::type
	as() const & {
		assert(type_is<T>());
		return **reinterpret_cast<const T *const *>(&_sto);
	}

	template <typename T, typename... Args>
	void emplace(Args &&... args) {
		reset();
		_emplace<T>(std::forward<Args>(args)...);
	}

	template <typename T, typename U, typename... Args>
	void emplace(std::initializer_list<U> il, Args &&... args) {
		reset();
		_emplace<T>(il, std::forward<Args>(args)...);
	}

	void reset() {
		if (!_typ_inf) {
			return;
		}
		if (_RUA_ANY_IS_DYNAMIC_ALLOCATION(
				_typ_inf->size, _typ_inf->align, StorageLen, StorageAlign)) {
			assert(_typ_inf->del);
			_typ_inf->del(*reinterpret_cast<uintptr_t *>(&_sto));
		} else if (_typ_inf->dtor) {
			_typ_inf->dtor(reinterpret_cast<void *>(&_sto));
		}
		_typ_inf = nullptr;
	}

private:
	alignas(StorageAlign) char _sto[StorageLen];
	const type_info_t *_typ_inf;

	template <typename T, typename... Args>
	RUA_FORCE_INLINE typename std::enable_if<
		!is_dynamic_allocation<typename std::decay<T>::type>::value>::type
	_emplace(Args &&... args) {
		new (&_sto) T(std::forward<Args>(args)...);
		_typ_inf = &type_info<T>();
	}

	template <typename T, typename... Args>
	RUA_FORCE_INLINE typename std::enable_if<
		is_dynamic_allocation<typename std::decay<T>::type>::value>::type
	_emplace(Args &&... args) {
		*reinterpret_cast<T **>(&_sto) = new T(std::forward<Args>(args)...);
		_typ_inf = &type_info<T>();
	}
};

using any = basic_any<sizeof(void *), alignof(void *)>;

template <class T, class... Args>
any make_any(Args &&... args) {
	return any(in_place_type_t<T>{}, std::forward<Args>(args)...);
}

template <class T, class U, class... Args>
any make_any(std::initializer_list<U> il, Args &&... args) {
	return any(in_place_type_t<T>{}, il, std::forward<Args>(args)...);
}

} // namespace rua

#endif
