#ifndef _rua_any_hpp
#define _rua_any_hpp

#include "type_info.hpp"
#include "util.hpp"

#include <cassert>

namespace rua {

template <size_t StorageSize, size_t StorageAlign>
class basic_any : public enable_type_info {
public:
	template <typename T>
	struct is_placeable : bool_constant<RUA_IS_PLACEABLE(
							  size_of<T>::value,
							  align_of<T>::value,
							  StorageSize,
							  StorageAlign)> {};

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

		$type.copy_to_storage<StorageSize, StorageAlign>(
			&$sto[0], &src.$sto[0]);
	}

	basic_any(basic_any &&src) : enable_type_info(src.$type) {
		if (!$type) {
			return;
		}

		assert($type.is_moveable());

		$type.move_to_storage<StorageSize, StorageAlign>(
			&$sto[0], &src.$sto[0]);
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
			is_placeable<decay_t<T>>::value,
		T &>
	as() & {
		assert(type_is<T>());
		return *reinterpret_cast<T *>(&$sto[0]);
	}

	template <typename T>
	enable_if_t<
		!std::is_void<T>::value && !is_null_pointer<T>::value &&
			!is_placeable<decay_t<T>>::value,
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
			is_placeable<decay_t<T>>::value,
		const T &>
	as() const & {
		assert(type_is<T>());
		return *reinterpret_cast<const T *>(&$sto[0]);
	}

	template <typename T>
	enable_if_t<
		!std::is_void<T>::value && !is_null_pointer<T>::value &&
			!is_placeable<decay_t<T>>::value,
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
		if ($type.is_placeable(StorageSize, StorageAlign)) {
			$type.destruct(reinterpret_cast<void *>(&$sto[0]));
		} else {
			$type.dealloc(*reinterpret_cast<void **>(&$sto[0]));
		}
		$type.reset();
	}

private:
	alignas(StorageAlign) uchar $sto[StorageSize];

	template <typename T, typename... Args>
	enable_if_t<is_placeable<T>::value, T &> $emplace(Args &&...args) {
		$type = type_id<T>();
		return construct(
			*reinterpret_cast<T *>(&$sto[0]), std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	enable_if_t<!is_placeable<T>::value, T &> $emplace(Args &&...args) {
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
