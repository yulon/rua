#ifndef _RUA_INTERFACE_PTR_HPP
#define _RUA_INTERFACE_PTR_HPP

#include "macros.hpp"
#include "types.hpp"

#include <cassert>
#include <memory>

namespace rua {

template <typename T>
class interface_ptr {
public:
	constexpr interface_ptr(std::nullptr_t = nullptr) :
		_raw(nullptr), _shared(), _type() {}

	template <RUA_IS_BASE_OF_CONCEPT(T, SameBase)>
	interface_ptr(SameBase *raw_ptr, type_info typ = nullptr) {
		if (!raw_ptr) {
			_raw = nullptr;
			return;
		}
		_type = typ ? typ : type_id<SameBase>();
		_raw = static_cast<T *>(raw_ptr);
	}

	template <RUA_IS_BASE_OF_CONCEPT(T, SameBase)>
	interface_ptr(SameBase &lv, type_info typ = nullptr) :
		interface_ptr(&lv, typ) {}

	template <RUA_IS_BASE_OF_CONCEPT(T, SameBase)>
	interface_ptr(SameBase &&rv, type_info typ = nullptr) {
		_type = typ ? typ : type_id<SameBase>();
		_shared = std::static_pointer_cast<T>(
			std::make_shared<SameBase>(std::move(rv)));
		_raw = _shared.get();
	}

	interface_ptr(std::shared_ptr<T> base_shared_ptr, type_info typ = nullptr) {
		if (!base_shared_ptr) {
			_raw = nullptr;
			return;
		}
		_type = typ ? typ : type_id<T>();
		_shared = std::move(base_shared_ptr);
		_raw = _shared.get();
	}

	template <RUA_DERIVED_CONCEPT(T, Derived)>
	interface_ptr(
		std::shared_ptr<Derived> derived_shared_ptr, type_info typ = nullptr) {
		if (!derived_shared_ptr) {
			_raw = nullptr;
			return;
		}
		_type = typ ? typ : type_id<Derived>();
		_shared = std::static_pointer_cast<T>(std::move(derived_shared_ptr));
		_raw = _shared.get();
	}

	template <RUA_IS_BASE_OF_CONCEPT(T, SameBase)>
	interface_ptr(
		const std::unique_ptr<SameBase> &unique_ptr_lv,
		type_info typ = nullptr) :
		interface_ptr(unique_ptr_lv.get(), typ) {}

	template <RUA_IS_BASE_OF_CONCEPT(T, SameBase)>
	interface_ptr(
		std::unique_ptr<SameBase> &&unique_ptr_rv, type_info typ = nullptr) {
		if (!unique_ptr_rv) {
			_raw = nullptr;
			return;
		}
		_type = typ ? typ : type_id<SameBase>();
		_shared.reset(static_cast<T *>(unique_ptr_rv.release()));
		_raw = _shared.get();
	}

	template <RUA_DERIVED_CONCEPT(T, Derived)>
	interface_ptr(const interface_ptr<Derived> &derived_interface_ptr_lv) :
		_type(derived_interface_ptr_lv.type()) {

		if (!derived_interface_ptr_lv) {
			_raw = nullptr;
			return;
		}

		auto shared = derived_interface_ptr_lv.get_shared();
		if (shared) {
			_shared = std::static_pointer_cast<T>(std::move(shared));
			_raw = _shared.get();
			return;
		}

		auto raw = derived_interface_ptr_lv.get();
		assert(raw);
		_raw = static_cast<T *>(raw);
	}

	template <RUA_DERIVED_CONCEPT(T, Derived)>
	interface_ptr(interface_ptr<Derived> &&derived_interface_ptr_rv) :
		_type(derived_interface_ptr_rv.type()) {

		if (!derived_interface_ptr_rv) {
			_raw = nullptr;
			return;
		}

		auto shared = derived_interface_ptr_rv.release_shared();
		if (shared) {
			_shared = std::static_pointer_cast<T>(std::move(shared));
			_raw = _shared.get();
			return;
		}

		auto raw = derived_interface_ptr_rv.release();
		assert(raw);
		_raw = static_cast<T *>(raw);
	}

	interface_ptr(const interface_ptr &src) :
		_raw(src._raw), _shared(src._shared), _type(src._type) {}

	interface_ptr(interface_ptr &&src) :
		_raw(src._raw), _shared(std::move(src._shared)), _type(src._type) {
		if (src._raw) {
			src._raw = nullptr;
		}
	}

	RUA_OVERLOAD_ASSIGNMENT(interface_ptr)

	~interface_ptr() {
		reset();
	}

	operator bool() const {
		return is_valid(_raw);
	}

	bool operator==(const interface_ptr<T> &src) const {
		return _raw == src._raw;
	}

	bool operator!=(const interface_ptr<T> &src) const {
		return _raw != src._raw;
	}

	template <
		typename =
			enable_if_t<std::is_class<T>::value || std::is_union<T>::value>>
	T *operator->() const {
		assert(_raw);
		return _raw;
	}

	T &operator*() const {
		return *_raw;
	}

	T *get() const {
		return _raw;
	}

	T *release() {
		if (_shared) {
			return nullptr;
		}
		auto raw = _raw;
		_raw = nullptr;
		return raw;
	}

	const std::shared_ptr<T> &get_shared() const {
		return _shared;
	}

	size_t use_count() const {
		return _shared.use_count();
	}

	std::shared_ptr<T> release_shared() {
		if (!_shared) {
			return nullptr;
		}
		_raw = nullptr;
		return std::move(_shared);
	}

	type_info type() const {
		return _raw ? _type : type_id<void>();
	}

	template <RUA_IS_BASE_OF_CONCEPT(T, SameBase)>
	bool type_is() const {
		return type() == type_id<SameBase>();
	}

	template <RUA_DERIVED_CONCEPT(T, Derived)>
	interface_ptr<Derived> as() const {
		assert(_raw);

		if (!type_is<Derived>()) {
			return nullptr;
		}
		if (!_shared) {
			if (!_raw) {
				return nullptr;
			}
			return static_cast<Derived *>(_raw);
		}
		return std::static_pointer_cast<Derived>(_shared);
	}

	void reset() {
		if (_shared) {
			_shared.reset();
		}
		_raw = nullptr;
	}

private:
	T *_raw;
	std::shared_ptr<T> _shared;
	type_info _type;
};

template <typename T>
class weak_interface {
public:
	constexpr weak_interface(std::nullptr_t = nullptr) :
		_raw(nullptr), _weak(), _type() {}

	weak_interface(const interface_ptr<T> &r) :
		_raw(r.get_shared() ? r.get() : nullptr),
		_weak(r.get_shared()),
		_type(r.type()) {}

	~weak_interface() {
		reset();
	}

	operator bool() const {
		return _raw || _weak.use_count();
	}

	interface_ptr<T> lock() const {
		if (_raw) {
			return interface_ptr<T>(_raw, _type);
		}
		if (_weak.use_count()) {
			return interface_ptr<T>(_weak.lock(), _type);
		}
		return nullptr;
	}

	void reset() {
		if (_weak) {
			_weak.reset();
		}
		_raw = nullptr;
	}

private:
	T *_raw;
	std::weak_ptr<T> _weak;
	type_info _type;
};

} // namespace rua

#endif
