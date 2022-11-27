#ifndef _RUA_ANY_HPP
#define _RUA_ANY_HPP

#include "type_info.hpp"
#include "util.hpp"

#include <cassert>

namespace rua {

#define _RUA_IS_CONTAINABLE(val_len, val_align, sto_len, sto_align)            \
	(val_len <= sto_len && val_align <= sto_align)

template <size_t StorageLen, size_t StorageAlign>
class basic_any : public enable_type_info {
public:
	template <typename T>
	struct is_containable {
		static constexpr auto value = _RUA_IS_CONTAINABLE(
			size_of<T>::value, align_of<T>::value, StorageLen, StorageAlign);
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
	explicit basic_any(in_place_type_t<T>, Args &&...args) {
		_emplace<T>(std::forward<Args>(args)...);
	}

	template <typename T, typename U, typename... Args>
	explicit basic_any(
		in_place_type_t<T>, std::initializer_list<U> il, Args &&...args) {
		_emplace<T>(il, std::forward<Args>(args)...);
	}

	~basic_any() {
		reset();
	}

	basic_any(const basic_any &src) : enable_type_info(src._type) {
		if (!_type) {
			return;
		}

		assert(_type.is_copyable());

		if (_RUA_IS_CONTAINABLE(
				_type.size(), _type.align(), StorageLen, StorageAlign)) {
			_type.copy_to(&_sto[0], &src._sto);
			return;
		}
		*reinterpret_cast<void **>(&_sto[0]) = _type.copy_to_new(
			*reinterpret_cast<const void *const *>(&src._sto));
	}

	basic_any(basic_any &&src) : enable_type_info(src._type) {
		if (!_type) {
			return;
		}

		assert(_type.is_moveable());

		if (_RUA_IS_CONTAINABLE(
				_type.size(), _type.align(), StorageLen, StorageAlign)) {
			_type.move_to(&_sto[0], &src._sto);
			return;
		}
		*reinterpret_cast<void **>(&_sto[0]) =
			*reinterpret_cast<void **>(&src._sto);
		src._type.reset();
	}

	RUA_OVERLOAD_ASSIGNMENT(basic_any)

	bool has_value() const {
		return _type;
	}

	operator bool() const {
		return _type;
	}

	template <typename T>
	enable_if_t<is_containable<decay_t<T>>::value, T &> as() & {
		assert(type_is<T>());
		return *reinterpret_cast<T *>(&_sto[0]);
	}

	template <typename T>
	enable_if_t<!is_containable<decay_t<T>>::value, T &> as() & {
		assert(type_is<T>());
		return **reinterpret_cast<T **>(&_sto[0]);
	}

	template <typename T>
	T &&as() && {
		return std::move(as<T>());
	}

	template <typename T>
	enable_if_t<is_containable<decay_t<T>>::value, const T &> as() const & {
		assert(type_is<T>());
		return *reinterpret_cast<const T *>(&_sto[0]);
	}

	template <typename T>
	enable_if_t<!is_containable<decay_t<T>>::value, const T &> as() const & {
		assert(type_is<T>());
		return **reinterpret_cast<const T *const *>(&_sto[0]);
	}

	template <typename T, typename Emplaced = decay_t<T &&>>
	Emplaced &emplace(T &&val) & {
		reset();
		return _emplace<Emplaced>(std::forward<T>(val));
	}

	template <typename T, typename Emplaced = decay_t<T &&>>
	Emplaced &&emplace(T &&val) && {
		reset();
		return std::move(_emplace<Emplaced>(std::forward<T>(val)));
	}

	template <typename T, typename... Args>
	T &emplace(Args &&...args) & {
		reset();
		return _emplace<T>(std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	T &&emplace(Args &&...args) && {
		return std::move(emplace<T>(std::forward<Args>(args)...));
	}

	template <typename T, typename U, typename... Args>
	T &emplace(std::initializer_list<U> il, Args &&...args) & {
		reset();
		return _emplace<T>(il, std::forward<Args>(args)...);
	}

	template <typename T, typename U, typename... Args>
	T &&emplace(std::initializer_list<U> il, Args &&...args) && {
		return std::move(emplace<T>(il, std::forward<Args>(args)...));
	}

	void reset() {
		if (!_type) {
			return;
		}
		if (_RUA_IS_CONTAINABLE(
				_type.size(), _type.align(), StorageLen, StorageAlign)) {
			_type.destruct(reinterpret_cast<void *>(&_sto[0]));
		} else {
			_type.dealloc(*reinterpret_cast<void **>(&_sto[0]));
		}
		_type.reset();
	}

private:
	alignas(StorageAlign) char _sto[StorageLen];

	template <typename T, typename... Args>
	enable_if_t<is_containable<T>::value, T &> _emplace(Args &&...args) {
		_type = type_id<T>();
		return construct(
			*reinterpret_cast<T *>(&_sto[0]), std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	enable_if_t<!is_containable<T>::value, T &> _emplace(Args &&...args) {
		_type = type_id<T>();
		return *(
			*reinterpret_cast<T **>(&_sto[0]) =
				new T(std::forward<Args>(args)...));
	}
};

using any = basic_any<sizeof(void *), alignof(void *)>;

template <typename T, typename... Args>
any make_any(Args &&...args) {
	return any(in_place_type_t<T>{}, std::forward<Args>(args)...);
}

template <typename T, typename U, typename... Args>
any make_any(std::initializer_list<U> il, Args &&...args) {
	return any(in_place_type_t<T>{}, il, std::forward<Args>(args)...);
}

} // namespace rua

#endif
