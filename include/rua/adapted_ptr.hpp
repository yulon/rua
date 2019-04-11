#ifndef _RUA_ADAPTED_PTR_HPP
#define _RUA_ADAPTED_PTR_HPP

#include "macros.hpp"
#include "type_traits.hpp"

#include <cstdint>
#include <memory>
#include <typeindex>
#include <cassert>

namespace rua {

template <typename T>
class adapted_ptr {
public:
	constexpr adapted_ptr(std::nullptr_t = nullptr) :
		_raw_ptr(nullptr), _shared_ptr(), _type(type_id<std::nullptr_t>())
	{}

	RUA_IS_BASE_OF_CONCEPT(T, SameBase)
	adapted_ptr(SameBase *raw_ptr) {
		if (!raw_ptr) {
			_raw_ptr = nullptr;
			_type = type_id<std::nullptr_t>();
			return;
		}
		_type = type_id<SameBase>();
		_raw_ptr = static_cast<T *>(raw_ptr);
	}

	RUA_IS_BASE_OF_CONCEPT(T, SameBase)
	adapted_ptr(SameBase &lv) : adapted_ptr(&lv) {}

	RUA_IS_BASE_OF_CONCEPT(T, SameBase)
	adapted_ptr(SameBase &&rv) {
		_type = type_id<SameBase>();
		new (&_shared_ptr) std::shared_ptr<T>(
			std::static_pointer_cast<T>(
				std::make_shared<SameBase>(std::move(rv))
			)
		);
		_raw_ptr = _shared_ptr.get();
	}

	adapted_ptr(const std::shared_ptr<T> &base_shared_ptr_lv) {
		if (!base_shared_ptr_lv) {
			_raw_ptr = nullptr;
			_type = type_id<std::nullptr_t>();
			return;
		}
		_type = type_id<T>();
		new (&_shared_ptr) std::shared_ptr<T>(
			base_shared_ptr_lv
		);
		_raw_ptr = _shared_ptr.get();
	}

	adapted_ptr(std::shared_ptr<T> &&base_shared_ptr_rv) {
		if (!base_shared_ptr_rv) {
			_raw_ptr = nullptr;
			_type = type_id<std::nullptr_t>();
			return;
		}
		_type = type_id<T>();
		new (&_shared_ptr) std::shared_ptr<T>(
			std::move(base_shared_ptr_rv)
		);
		_raw_ptr = _shared_ptr.get();
	}

	RUA_DERIVED_CONCEPT(T, Derived)
	adapted_ptr(const std::shared_ptr<Derived> &derived_shared_ptr_lv) {
		if (!derived_shared_ptr_lv) {
			_raw_ptr = nullptr;
			_type = type_id<std::nullptr_t>();
			return;
		}
		_type = type_id<Derived>();
		new (&_shared_ptr) std::shared_ptr<T>(
			std::static_pointer_cast<T>(derived_shared_ptr_lv)
		);
		_raw_ptr = _shared_ptr.get();
	}

	RUA_DERIVED_CONCEPT(T, Derived)
	adapted_ptr(std::shared_ptr<Derived> &&derived_shared_ptr_rv) {
		if (!derived_shared_ptr_rv) {
			_raw_ptr = nullptr;
			_type = type_id<std::nullptr_t>();
			return;
		}
		_type = type_id<Derived>();
		new (&_shared_ptr) std::shared_ptr<T>(
			std::static_pointer_cast<T>(std::move(derived_shared_ptr_rv))
		);
		_raw_ptr = _shared_ptr.get();
	}

	RUA_IS_BASE_OF_CONCEPT(T, SameBase)
	adapted_ptr(const std::unique_ptr<SameBase> &unique_ptr_lv) : adapted_ptr(unique_ptr_lv.get()) {}

	RUA_IS_BASE_OF_CONCEPT(T, SameBase)
	adapted_ptr(std::unique_ptr<SameBase> &&unique_ptr_rv) {
		if (!unique_ptr_rv) {
			_raw_ptr = nullptr;
			_type = type_id<std::nullptr_t>();
			return;
		}
		_type = type_id<SameBase>();
		new (&_shared_ptr) std::shared_ptr<T>(
			static_cast<T *>(unique_ptr_rv.release())
		);
		_raw_ptr = _shared_ptr.get();
	}

	RUA_DERIVED_CONCEPT(T, Derived)
	adapted_ptr(const adapted_ptr<Derived> &derived_interface_ptr_lv) {
		_type = derived_interface_ptr_lv.type();

		if (!derived_interface_ptr_lv) {
			_raw_ptr = nullptr;
			return;
		}

		auto src_base_ptr_shared_ptr = derived_interface_ptr_lv.get_shared();
		if (src_base_ptr_shared_ptr) {
			new (&_shared_ptr) std::shared_ptr<T>(
				std::static_pointer_cast<T>(std::move(src_base_ptr_shared_ptr))
			);
			_raw_ptr = _shared_ptr.get();
			return;
		}

		_raw_ptr = static_cast<T *>(derived_interface_ptr_lv.get());
	}

	RUA_DERIVED_CONCEPT(T, Derived)
	adapted_ptr(adapted_ptr<Derived> &&derived_interface_ptr_rv) {
		_type = derived_interface_ptr_rv.type();

		if (!derived_interface_ptr_rv) {
			_raw_ptr = nullptr;
			return;
		}

		auto src_base_ptr_shared_ptr = derived_interface_ptr_rv.release_shared();
		if (src_base_ptr_shared_ptr) {
			new (&_shared_ptr) std::shared_ptr<T>(
				std::static_pointer_cast<T>(std::move(src_base_ptr_shared_ptr))
			);
			_raw_ptr = _shared_ptr.get();
			return;
		}

		_raw_ptr = static_cast<T *>(derived_interface_ptr_rv.release());
	}

	adapted_ptr(const adapted_ptr<T> &src) {
		_raw_ptr = src._raw_ptr;
		_type = src._type;

		if (!_raw_ptr || !src._shared_ptr) {
			return;
		}

		_shared_ptr = src._shared_ptr;
	}

	adapted_ptr<T> &operator=(const adapted_ptr<T> &src) {
		reset();
		new (this) adapted_ptr<T>(src);
		return *this;
	}

	adapted_ptr(adapted_ptr<T> &&src) {
		_type = src._type;

		_raw_ptr = src.release();
		if (_raw_ptr || !src) {
			return;
		}

		_shared_ptr = src.release_shared();
		_raw_ptr = _shared_ptr.get();
	}

	adapted_ptr<T> &operator=(adapted_ptr<T> &&src) {
		reset();
		new (this) adapted_ptr<T>(std::move(src));
		return *this;
	}

	~adapted_ptr() {
		reset();
	}

	operator bool() const {
		return _raw_ptr;
	}

	bool operator==(const adapted_ptr<T> &src) const {
		return _raw_ptr == src._raw_ptr;
	}

	bool operator!=(const adapted_ptr<T> &src) const {
		return _raw_ptr != src._raw_ptr;
	}

	T *operator->() const {
		assert(_raw_ptr);
		return _raw_ptr;
	}

	T &operator*() const {
		return *_raw_ptr;
	}

	uintptr_t type() const {
		return _type;
	}

	RUA_IS_BASE_OF_CONCEPT(T, SameBase)
	bool type_is() const {
		return type() == type_id<SameBase>();
	}

	T *get() const {
		return _raw_ptr;
	}

	T *release() {
		if (_shared_ptr) {
			return nullptr;
		}
		auto base_raw_ptr = _raw_ptr;
		_raw_ptr = nullptr;
		_type = type_id<std::nullptr_t>();
		return base_raw_ptr;
	}

	std::shared_ptr<T> get_shared() const & {
		if (_shared_ptr) {
			return _shared_ptr;
		}
		return nullptr;
	}

	std::shared_ptr<T> release_shared() {
		if (!_shared_ptr) {
			return nullptr;
		}
		_raw_ptr = nullptr;
		_type = type_id<std::nullptr_t>();
		return std::move(_shared_ptr);
	}

	std::shared_ptr<T> get_shared() && {
		return release_shared();
	}

	RUA_IS_BASE_OF_CONCEPT(T, SameBase)
	SameBase *to() const {
		assert(_raw_ptr);

		if (!type_is<SameBase>()) {
			return nullptr;
		}
		if (!_raw_ptr) {
			return nullptr;
		}
		return static_cast<SameBase *>(_raw_ptr);
	}

	RUA_IS_BASE_OF_CONCEPT(T, SameBase)
	std::shared_ptr<SameBase> to_shared() const {
		assert(_raw_ptr);

		if (!type_is<SameBase>()) {
			return nullptr;
		}
		auto base_ptr = get_shared();
		if (!base_ptr) {
			return nullptr;
		}
		return std::static_pointer_cast<SameBase>(base_ptr);
	}

	void reset() {
		if (_shared_ptr) {
			_shared_ptr.reset();
		}
		_raw_ptr = nullptr;
		_type = type_id<std::nullptr_t>();
	}

private:
	T *_raw_ptr;
	std::shared_ptr<T> _shared_ptr;
	uintptr_t _type;
};

}

#endif
