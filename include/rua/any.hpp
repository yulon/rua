#ifndef _rua_any_hpp
#define _rua_any_hpp

#include "type_info.hpp"
#include "util.hpp"

#include <cassert>

namespace rua {

#define RUA_IS_CONTAINABLE(val_len, val_align, sto_len, sto_align)             \
	(val_len <= sto_len && val_align <= sto_align)

template <size_t StorageLen, size_t StorageAlign>
class basic_any : public enable_type_info {
public:
	template <typename T>
	struct is_containable {
		static constexpr auto value = RUA_IS_CONTAINABLE(
			size_of<T>::value, align_of<T>::value, StorageLen, StorageAlign);
	};

	////////////////////////////////////////////////////////////////////////

	constexpr basic_any() : enable_type_info(), $sto() {}

	template <
		typename T,
		typename = enable_if_t<!std::is_base_of<basic_any, decay_t<T>>::value>>
	basic_any(T &&val) {
		$emplace<decay_t<T>>(std::forward<T>(val));
	}

	template <typename T, typename... Args>
	explicit basic_any(in_place_type_t<T>, Args &&...args) {
		$emplace<T>(std::forward<Args>(args)...);
	}

	template <typename T, typename U, typename... Args>
	explicit basic_any(
		in_place_type_t<T>, std::initializer_list<U> il, Args &&...args) {
		$emplace<T>(il, std::forward<Args>(args)...);
	}

	~basic_any() {
		reset();
	}

	basic_any(const basic_any &src) : enable_type_info(src.$type) {
		if (!$type) {
			return;
		}

		assert($type.is_copyable());

		if (RUA_IS_CONTAINABLE(
				$type.size(), $type.align(), StorageLen, StorageAlign)) {
			$type.copy_to(&$sto[0], &src.$sto);
			return;
		}
		*reinterpret_cast<void **>(&$sto[0]) = $type.copy_to_new(
			*reinterpret_cast<const void *const *>(&src.$sto));
	}

	basic_any(basic_any &&src) : enable_type_info(src.$type) {
		if (!$type) {
			return;
		}

		assert($type.is_moveable());

		if (RUA_IS_CONTAINABLE(
				$type.size(), $type.align(), StorageLen, StorageAlign)) {
			$type.move_to(&$sto[0], &src.$sto);
			return;
		}
		*reinterpret_cast<void **>(&$sto[0]) =
			*reinterpret_cast<void **>(&src.$sto);
		src.$type.reset();
	}

	RUA_OVERLOAD_ASSIGNMENT(basic_any)

	bool has_value() const {
		return $type;
	}

	operator bool() const {
		return $type;
	}

	template <typename T>
	enable_if_t<
		!std::is_void<T>::value && !is_null_pointer<T>::value &&
			is_containable<decay_t<T>>::value,
		T &>
	as() & {
		assert(type_is<T>());
		return *reinterpret_cast<T *>(&$sto[0]);
	}

	template <typename T>
	enable_if_t<
		!std::is_void<T>::value && !is_null_pointer<T>::value &&
			!is_containable<decay_t<T>>::value,
		T &>
	as() & {
		assert(type_is<T>());
		return **reinterpret_cast<T **>(&$sto[0]);
	}

	template <typename T>
	enable_if_t<!std::is_void<T>::value && !is_null_pointer<T>::value, T &&>
	as() && {
		return std::move(as<T>());
	}

	template <typename T>
	enable_if_t<
		!std::is_void<T>::value && !is_null_pointer<T>::value &&
			is_containable<decay_t<T>>::value,
		const T &>
	as() const & {
		assert(type_is<T>());
		return *reinterpret_cast<const T *>(&$sto[0]);
	}

	template <typename T>
	enable_if_t<
		!std::is_void<T>::value && !is_null_pointer<T>::value &&
			!is_containable<decay_t<T>>::value,
		const T &>
	as() const & {
		assert(type_is<T>());
		return **reinterpret_cast<const T *const *>(&$sto[0]);
	}

	template <typename T>
	RUA_CONSTEXPR_14 enable_if_t<std::is_void<T>::value> as() const & {}

	template <typename T>
	constexpr enable_if_t<is_null_pointer<T>::value, std::nullptr_t>
	as() const & {
		return nullptr;
	}

	template <typename T, typename Emplaced = decay_t<T &&>>
	Emplaced &emplace(T &&val) & {
		reset();
		return $emplace<Emplaced>(std::forward<T>(val));
	}

	template <typename T, typename Emplaced = decay_t<T &&>>
	Emplaced &&emplace(T &&val) && {
		reset();
		return std::move($emplace<Emplaced>(std::forward<T>(val)));
	}

	template <typename T, typename... Args>
	T &emplace(Args &&...args) & {
		reset();
		return $emplace<T>(std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	T &&emplace(Args &&...args) && {
		return std::move(emplace<T>(std::forward<Args>(args)...));
	}

	template <typename T, typename U, typename... Args>
	T &emplace(std::initializer_list<U> il, Args &&...args) & {
		reset();
		return $emplace<T>(il, std::forward<Args>(args)...);
	}

	template <typename T, typename U, typename... Args>
	T &&emplace(std::initializer_list<U> il, Args &&...args) && {
		return std::move(emplace<T>(il, std::forward<Args>(args)...));
	}

	void reset() {
		if (!$type) {
			return;
		}
		if (RUA_IS_CONTAINABLE(
				$type.size(), $type.align(), StorageLen, StorageAlign)) {
			$type.destruct(reinterpret_cast<void *>(&$sto[0]));
		} else {
			$type.dealloc(*reinterpret_cast<void **>(&$sto[0]));
		}
		$type.reset();
	}

private:
	alignas(StorageAlign) uchar $sto[StorageLen];

	template <typename T, typename... Args>
	enable_if_t<is_containable<T>::value, T &> $emplace(Args &&...args) {
		$type = type_id<T>();
		return construct(
			*reinterpret_cast<T *>(&$sto[0]), std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	enable_if_t<!is_containable<T>::value, T &> $emplace(Args &&...args) {
		$type = type_id<T>();
		return *(
			*reinterpret_cast<T **>(&$sto[0]) =
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
