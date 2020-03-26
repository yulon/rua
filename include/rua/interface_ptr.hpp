#ifndef _RUA_INTERFACE_PTR_HPP
#define _RUA_INTERFACE_PTR_HPP

#include "macros.hpp"
#include "types.hpp"

#include <cassert>
#include <memory>

namespace rua {

template <typename T>
class interface_ptr : public enable_ptr_type_info<interface_ptr<T>> {
public:
	constexpr interface_ptr(std::nullptr_t = nullptr) :
		enable_ptr_type_info<interface_ptr<T>>(), _raw(nullptr), _shared() {}

	template <RUA_IS_BASE_OF_CONCEPT(T, SameBase)>
	interface_ptr(SameBase *raw_ptr) {
		if (!raw_ptr) {
			_raw = nullptr;
			return;
		}
		this->_type = type_id<SameBase>();
		_raw = static_cast<T *>(raw_ptr);
	}

	template <RUA_IS_BASE_OF_CONCEPT(T, SameBase)>
	interface_ptr(SameBase &lv) : interface_ptr(&lv) {}

	template <RUA_IS_BASE_OF_CONCEPT(T, SameBase)>
	interface_ptr(SameBase &&rv) {
		this->_type = type_id<SameBase>();
		new (&_shared) std::shared_ptr<T>(std::static_pointer_cast<T>(
			std::make_shared<SameBase>(std::move(rv))));
		_raw = _shared.get();
	}

	interface_ptr(const std::shared_ptr<T> &base_shared_ptr_lv) {
		if (!base_shared_ptr_lv) {
			_raw = nullptr;
			return;
		}
		this->_type = type_id<T>();
		new (&_shared) std::shared_ptr<T>(base_shared_ptr_lv);
		_raw = _shared.get();
	}

	interface_ptr(std::shared_ptr<T> &&base_shared_ptr_rv) {
		if (!base_shared_ptr_rv) {
			_raw = nullptr;
			return;
		}
		this->_type = type_id<T>();
		new (&_shared) std::shared_ptr<T>(std::move(base_shared_ptr_rv));
		_raw = _shared.get();
	}

	template <RUA_DERIVED_CONCEPT(T, Derived)>
	interface_ptr(const std::shared_ptr<Derived> &derived_shared_ptr_lv) {
		if (!derived_shared_ptr_lv) {
			_raw = nullptr;
			return;
		}
		this->_type = type_id<Derived>();
		new (&_shared) std::shared_ptr<T>(
			std::static_pointer_cast<T>(derived_shared_ptr_lv));
		_raw = _shared.get();
	}

	template <RUA_DERIVED_CONCEPT(T, Derived)>
	interface_ptr(std::shared_ptr<Derived> &&derived_shared_ptr_rv) {
		if (!derived_shared_ptr_rv) {
			_raw = nullptr;
			return;
		}
		this->_type = type_id<Derived>();
		new (&_shared) std::shared_ptr<T>(
			std::static_pointer_cast<T>(std::move(derived_shared_ptr_rv)));
		_raw = _shared.get();
	}

	template <RUA_IS_BASE_OF_CONCEPT(T, SameBase)>
	interface_ptr(const std::unique_ptr<SameBase> &unique_ptr_lv) :
		interface_ptr(unique_ptr_lv.get()) {}

	template <RUA_IS_BASE_OF_CONCEPT(T, SameBase)>
	interface_ptr(std::unique_ptr<SameBase> &&unique_ptr_rv) {
		if (!unique_ptr_rv) {
			_raw = nullptr;
			return;
		}
		this->_type = type_id<SameBase>();
		new (&_shared)
			std::shared_ptr<T>(static_cast<T *>(unique_ptr_rv.release()));
		_raw = _shared.get();
	}

	template <RUA_DERIVED_CONCEPT(T, Derived)>
	interface_ptr(const interface_ptr<Derived> &derived_blended_ptr_lv) :
		enable_ptr_type_info<interface_ptr<T>>(derived_blended_ptr_lv.type()) {

		if (!derived_blended_ptr_lv) {
			_raw = nullptr;
			return;
		}

		auto src_base_ptr_shared_ptr = derived_blended_ptr_lv.get_shared();
		if (src_base_ptr_shared_ptr) {
			new (&_shared) std::shared_ptr<T>(std::static_pointer_cast<T>(
				std::move(src_base_ptr_shared_ptr)));
			_raw = _shared.get();
			return;
		}

		_raw = static_cast<T *>(derived_blended_ptr_lv.get());
	}

	template <RUA_DERIVED_CONCEPT(T, Derived)>
	interface_ptr(interface_ptr<Derived> &&derived_blended_ptr_rv) :
		enable_ptr_type_info<interface_ptr<T>>(derived_blended_ptr_rv.type()) {

		if (!derived_blended_ptr_rv) {
			_raw = nullptr;
			return;
		}

		auto src_base_ptr_shared_ptr = derived_blended_ptr_rv.release_shared();
		if (src_base_ptr_shared_ptr) {
			new (&_shared) std::shared_ptr<T>(std::static_pointer_cast<T>(
				std::move(src_base_ptr_shared_ptr)));
			_raw = _shared.get();
			return;
		}

		_raw = static_cast<T *>(derived_blended_ptr_rv.release());
	}

	interface_ptr(const interface_ptr &src) :
		enable_ptr_type_info<interface_ptr<T>>(src._type) {
		_raw = src._raw;
		if (!_raw || !src._shared) {
			return;
		}
		_shared = src._shared;
	}

	interface_ptr(interface_ptr &&src) :
		enable_ptr_type_info<interface_ptr<T>>(src._type) {
		_raw = src.release();
		if (_raw || !src) {
			return;
		}
		_shared = src.release_shared();
		_raw = _shared.get();
	}

	RUA_OVERLOAD_ASSIGNMENT(interface_ptr)

	~interface_ptr() {
		reset();
	}

	operator bool() const {
		return _raw;
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
		auto base_raw_ptr = _raw;
		_raw = nullptr;
		return base_raw_ptr;
	}

	std::shared_ptr<T> get_shared() const & {
		if (_shared) {
			return _shared;
		}
		return nullptr;
	}

	std::shared_ptr<T> release_shared() {
		if (!_shared) {
			return nullptr;
		}
		_raw = nullptr;
		return std::move(_shared);
	}

	std::shared_ptr<T> get_shared() && {
		return release_shared();
	}

	template <RUA_IS_BASE_OF_CONCEPT(T, SameBase)>
	SameBase *as() const {
		assert(_raw);

		if (!this->template type_is<SameBase>()) {
			return nullptr;
		}
		if (!_raw) {
			return nullptr;
		}
		return static_cast<SameBase *>(_raw);
	}

	template <RUA_IS_BASE_OF_CONCEPT(T, SameBase)>
	std::shared_ptr<SameBase> to_shared() const {
		assert(_raw);

		if (!this->template type_is<SameBase>()) {
			return nullptr;
		}
		auto base_ptr = get_shared();
		if (!base_ptr) {
			return nullptr;
		}
		return std::static_pointer_cast<SameBase>(base_ptr);
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
};

} // namespace rua

#endif
