#ifndef _RUA_ANY_HPP
#define _RUA_ANY_HPP

#include "types.hpp"

#include <cassert>

namespace rua {

#define _RUA_ANY_IS_DYNAMIC_ALLOCATION(val_len, val_align, sto_len, sto_align) \
	(val_len > sto_len || val_align > sto_align)

template <size_t StorageLen, size_t StorageAlign>
class basic_any : public enable_type_info {
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

	constexpr basic_any() : enable_type_info(), _sto() {}

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

	basic_any(const basic_any &src) :
		enable_type_info(static_cast<const enable_type_info &>(src)) {
		if (!has_value()) {
			return;
		}

		assert(_type.is_copyable());

		if (_RUA_ANY_IS_DYNAMIC_ALLOCATION(
				_type.size(), _type.align(), StorageLen, StorageAlign)) {
			*reinterpret_cast<void **>(&_sto[0]) = _type.copy_new(
				*reinterpret_cast<const void *const *>(&src._sto));
			return;
		}
		_type.copy_ctor(&_sto[0], &src._sto);
	}

	basic_any(basic_any &&src) :
		enable_type_info(static_cast<const enable_type_info &>(src)) {
		if (!has_value()) {
			return;
		}

		assert(_type.is_moveable());

		if (_RUA_ANY_IS_DYNAMIC_ALLOCATION(
				_type.size(), _type.align(), StorageLen, StorageAlign)) {
			*reinterpret_cast<void **>(&_sto[0]) =
				*reinterpret_cast<void **>(&src._sto);
			src._type = type_id<void>();
			return;
		}
		_type.move_ctor(&_sto[0], &src._sto);
	}

	RUA_OVERLOAD_ASSIGNMENT(basic_any)

	bool has_value() const {
		return !type_is<void>();
	}

	operator bool() const {
		return has_value();
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
		if (!has_value()) {
			return;
		}
		if (_RUA_ANY_IS_DYNAMIC_ALLOCATION(
				_type.size(), _type.align(), StorageLen, StorageAlign)) {
			_type.del(*reinterpret_cast<void **>(&_sto[0]));
		} else if (!_type.is_trivial()) {
			_type.dtor(reinterpret_cast<void *>(&_sto[0]));
		}
		_type = type_id<void>();
	}

private:
	alignas(StorageAlign) char _sto[StorageLen];

	template <typename T, typename... Args>
	RUA_FORCE_INLINE enable_if_t<!is_dynamic_allocation<T>::value, T &>
	_emplace(Args &&... args) {
		_type = type_id<T>();
		return *(new (&_sto[0]) T(std::forward<Args>(args)...));
	}

	template <typename T, typename... Args>
	RUA_FORCE_INLINE enable_if_t<is_dynamic_allocation<T>::value, T &>
	_emplace(Args &&... args) {
		_type = type_id<T>();
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
