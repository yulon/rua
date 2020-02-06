#ifndef _RUA_ANY_HPP
#define _RUA_ANY_HPP

#include "type_traits/measures.hpp"
#include "type_traits/std_patch.hpp"
#include "type_traits/type_info.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
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
		typename = enable_if_t<!std::is_base_of<basic_any, decay_t<T>>::value>>
	basic_any(T &&val) {
		_emplace<decay_t<T>>(std::forward<T>(val));
	}

	template <typename T, typename... Args>
	explicit basic_any(in_place_type_t<T>, Args &&... args) {
		_emplace<T>(std::forward<Args>(args)...);
	}

	template <typename T, typename U, typename... Args>
	explicit basic_any(
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
			*reinterpret_cast<uintptr_t *>(&_sto[0]) = _typ_inf->new_copy(
				*reinterpret_cast<const void *const *>(&src._sto));
			return;
		}
		_typ_inf->copy_ctor(&_sto[0], &src._sto);
	}

	basic_any(basic_any &&src) : _typ_inf(src._typ_inf) {
		if (!_typ_inf) {
			return;
		}

		assert(_typ_inf->move_ctor);

		if (_RUA_ANY_IS_DYNAMIC_ALLOCATION(
				_typ_inf->size, _typ_inf->align, StorageLen, StorageAlign)) {
			*reinterpret_cast<void **>(&_sto[0]) =
				*reinterpret_cast<void **>(&src._sto);
			src._typ_inf = nullptr;
			return;
		}
		_typ_inf->move_ctor(&_sto[0], &src._sto);
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
	enable_if_t<!is_dynamic_allocation<decay_t<T>>::value, T &> as() & {
		assert(type_is<T>());
		return *reinterpret_cast<T *>(&_sto[0]);
	}

	template <typename T>
	enable_if_t<is_dynamic_allocation<decay_t<T>>::value, T &> as() & {
		assert(type_is<T>());
		return **reinterpret_cast<T **>(&_sto[0]);
	}

	template <typename T>
	T &&as() && {
		return std::move(as<T>());
	}

	template <typename T>
	enable_if_t<!is_dynamic_allocation<decay_t<T>>::value, const T &>
	as() const & {
		assert(type_is<T>());
		return *reinterpret_cast<const T *>(&_sto[0]);
	}

	template <typename T>
	enable_if_t<is_dynamic_allocation<decay_t<T>>::value, const T &>
	as() const & {
		assert(type_is<T>());
		return **reinterpret_cast<const T *const *>(&_sto[0]);
	}

	template <typename T, typename... Args>
	T &emplace(Args &&... args) & {
		reset();
		return _emplace<T>(std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	T &&emplace(Args &&... args) && {
		return std::move(emplace<T>(std::forward<Args>(args)...));
	}

	template <typename T, typename U, typename... Args>
	T &emplace(std::initializer_list<U> il, Args &&... args) & {
		reset();
		return _emplace<T>(il, std::forward<Args>(args)...);
	}

	template <typename T, typename U, typename... Args>
	T &&emplace(std::initializer_list<U> il, Args &&... args) && {
		return std::move(emplace<T>(il, std::forward<Args>(args)...));
	}

	void reset() {
		if (!_typ_inf) {
			return;
		}
		if (_RUA_ANY_IS_DYNAMIC_ALLOCATION(
				_typ_inf->size, _typ_inf->align, StorageLen, StorageAlign)) {
			assert(_typ_inf->del);
			_typ_inf->del(*reinterpret_cast<uintptr_t *>(&_sto[0]));
		} else if (_typ_inf->dtor) {
			_typ_inf->dtor(reinterpret_cast<void *>(&_sto[0]));
		}
		_typ_inf = nullptr;
	}

private:
	alignas(StorageAlign) char _sto[StorageLen];
	const type_info_t *_typ_inf;

	template <typename T, typename... Args>
	RUA_FORCE_INLINE enable_if_t<!is_dynamic_allocation<T>::value, T &>
	_emplace(Args &&... args) {
		_typ_inf = &type_info<T>();
		return *(new (&_sto[0]) T(std::forward<Args>(args)...));
	}

	template <typename T, typename... Args>
	RUA_FORCE_INLINE enable_if_t<is_dynamic_allocation<T>::value, T &>
	_emplace(Args &&... args) {
		_typ_inf = &type_info<T>();
		return *(
			*reinterpret_cast<T **>(&_sto[0]) =
				new T(std::forward<Args>(args)...));
	}
};

using any = basic_any<sizeof(void *), alignof(void *)>;

template <typename T, typename... Args>
any make_any(Args &&... args) {
	return any(in_place_type_t<T>{}, std::forward<Args>(args)...);
}

template <typename T, typename U, typename... Args>
any make_any(std::initializer_list<U> il, Args &&... args) {
	return any(in_place_type_t<T>{}, il, std::forward<Args>(args)...);
}

} // namespace rua

#endif
