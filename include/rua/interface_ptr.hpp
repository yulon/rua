#ifndef _RUA_INTERFACE_HPP
#define _RUA_INTERFACE_HPP

#include "macros.hpp"
#include "type_traits.hpp"

#include <cstdint>
#include <memory>
#include <typeindex>
#include <cassert>

namespace rua {

#define _RUA_INTERFACE_CONCEPT(_T) template < \
	typename _T, \
	typename = typename std::enable_if<std::is_base_of<Base, typename std::remove_reference<_T>::type>::value>::type \
>
#define _RUA_INTERFACE_CONCEPT_2(_T, _cond) template < \
	typename _T, \
	typename = typename std::enable_if<std::is_base_of<Base, typename std::remove_reference<_T>::type>::value && _cond>::type \
>

template <typename Base>
class interface_ptr {
public:
	constexpr interface_ptr(std::nullptr_t = nullptr) :
		_base_raw_ptr(nullptr), _base_shared_ptr(), _derived_type(type_id<std::nullptr_t>())
	{}

	_RUA_INTERFACE_CONCEPT(Derived)
	interface_ptr(Derived *derived_ptr) {
		if (!derived_ptr) {
			_base_raw_ptr = nullptr;
			_derived_type = type_id<std::nullptr_t>();
			return;
		}
		_derived_type = type_id<Derived>();
		_base_raw_ptr = static_cast<Base *>(derived_ptr);
	}

	_RUA_INTERFACE_CONCEPT(Derived)
	interface_ptr(Derived &derived) : interface_ptr(&derived) {}

	_RUA_INTERFACE_CONCEPT(Derived)
	interface_ptr(Derived &&derived) {
		_derived_type = type_id<Derived>();
		new (&_base_shared_ptr) std::shared_ptr<Base>(
			std::static_pointer_cast<Base>(
				std::make_shared<Derived>(std::move(derived))
			)
		);
		_base_raw_ptr = _base_shared_ptr.get();
	}

	interface_ptr(const std::shared_ptr<Base> &derived_shared_ptr) {
		if (!derived_shared_ptr) {
			_base_raw_ptr = nullptr;
			_derived_type = type_id<std::nullptr_t>();
			return;
		}
		_derived_type = type_id<Base>();
		new (&_base_shared_ptr) std::shared_ptr<Base>(derived_shared_ptr);
		_base_raw_ptr = _base_shared_ptr.get();
	}

	interface_ptr(std::shared_ptr<Base> &&derived_shared_ptr) {
		if (!derived_shared_ptr) {
			_base_raw_ptr = nullptr;
			_derived_type = type_id<std::nullptr_t>();
			return;
		}
		_derived_type = type_id<Base>();
		new (&_base_shared_ptr) std::shared_ptr<Base>(std::move(derived_shared_ptr));
		_base_raw_ptr = _base_shared_ptr.get();
	}

	_RUA_INTERFACE_CONCEPT_2(Derived, (!std::is_same<Derived, Base>::value))
	interface_ptr(const std::shared_ptr<Derived> &derived_shared_ptr) : interface_ptr(derived_shared_ptr ? std::static_pointer_cast<Base>(derived_shared_ptr) : std::shared_ptr<Base>()) {}

	_RUA_INTERFACE_CONCEPT_2(Derived, (!std::is_same<Derived, Base>::value))
	interface_ptr(std::shared_ptr<Derived> &&derived_shared_ptr) : interface_ptr(derived_shared_ptr ? std::static_pointer_cast<Base>(std::move(derived_shared_ptr)) : std::shared_ptr<Base>()) {}

	_RUA_INTERFACE_CONCEPT(Derived)
	interface_ptr(const std::unique_ptr<Derived> &derived_unique_ptr) : interface_ptr(derived_unique_ptr.get()) {}

	_RUA_INTERFACE_CONCEPT(Derived)
	interface_ptr(std::unique_ptr<Derived> &&derived_unique_ptr) : interface_ptr(derived_unique_ptr ? std::shared_ptr<Base>(static_cast<Base *>(derived_unique_ptr.release())) : std::shared_ptr<Base>()) {}

	_RUA_INTERFACE_CONCEPT_2(OthrAbst, (!std::is_same<OthrAbst, Base>::value))
	interface_ptr(const interface_ptr<OthrAbst> &src) {
		_derived_type = src.type();

		if (!src) {
			_base_raw_ptr = nullptr;
			return;
		}

		auto src_base_ptr_shared_ptr = src.get_shared();
		if (src_base_ptr_shared_ptr) {
			new (&_base_shared_ptr) std::shared_ptr<Base>(
				std::static_pointer_cast<Base>(std::move(src_base_ptr_shared_ptr))
			);
			_base_raw_ptr = _base_shared_ptr.get();
			return;
		}

		_base_raw_ptr = static_cast<Base *>(src.get());
	}

	_RUA_INTERFACE_CONCEPT_2(OthrAbst, (!std::is_same<OthrAbst, Base>::value))
	interface_ptr(interface_ptr<OthrAbst> &&src) {
		_derived_type = src.type();

		if (!src) {
			_base_raw_ptr = nullptr;
			return;
		}

		auto src_base_ptr_shared_ptr = src.release_shared();
		if (src_base_ptr_shared_ptr) {
			new (&_base_shared_ptr) std::shared_ptr<Base>(
				std::static_pointer_cast<Base>(std::move(src_base_ptr_shared_ptr))
			);
			_base_raw_ptr = _base_shared_ptr.get();
			return;
		}

		_base_raw_ptr = static_cast<Base *>(src.release());
	}

	interface_ptr(const interface_ptr<Base> &src) {
		_base_raw_ptr = src._base_raw_ptr;
		_derived_type = src._derived_type;

		if (!_base_raw_ptr || !src._base_shared_ptr) {
			return;
		}

		_base_shared_ptr = src._base_shared_ptr;
	}

	interface_ptr<Base> &operator=(const interface_ptr<Base> &src) {
		reset();
		new (this) interface_ptr<Base>(src);
		return *this;
	}

	interface_ptr(interface_ptr<Base> &&src) {
		_derived_type = src._derived_type;

		_base_raw_ptr = src.release();
		if (_base_raw_ptr || !src) {
			return;
		}

		_base_shared_ptr = src.release_shared();
		_base_raw_ptr = _base_shared_ptr.get();
	}

	interface_ptr<Base> &operator=(interface_ptr<Base> &&src) {
		reset();
		new (this) interface_ptr<Base>(std::move(src));
		return *this;
	}

	~interface_ptr() {
		reset();
	}

	operator bool() const {
		return _base_raw_ptr;
	}

	bool operator==(const interface_ptr<Base> &src) const {
		return _base_raw_ptr == src._base_raw_ptr;
	}

	bool operator!=(const interface_ptr<Base> &src) const {
		return _base_raw_ptr != src._base_raw_ptr;
	}

	Base *operator->() const {
		return _base_raw_ptr;
	}

	Base &operator*() const {
		return *_base_raw_ptr;
	}

	uintptr_t type() const {
		return _derived_type;
	}

	_RUA_INTERFACE_CONCEPT(Derived)
	bool type_is() const {
		return type() == type_id<Derived>();
	}

	Base *get() const {
		return _base_raw_ptr;
	}

	Base *release() {
		if (_base_shared_ptr) {
			return nullptr;
		}
		auto base_raw_ptr = _base_raw_ptr;
		_base_raw_ptr = nullptr;
		_derived_type = type_id<std::nullptr_t>();
		return base_raw_ptr;
	}

	std::shared_ptr<Base> get_shared() const & {
		if (_base_shared_ptr) {
			return _base_shared_ptr;
		}
		return nullptr;
	}

	std::shared_ptr<Base> release_shared() {
		if (!_base_shared_ptr) {
			return nullptr;
		}
		_base_raw_ptr = nullptr;
		_derived_type = type_id<std::nullptr_t>();
		return std::move(_base_shared_ptr);
	}

	std::shared_ptr<Base> get_shared() && {
		return release_shared();
	}

	_RUA_INTERFACE_CONCEPT(Derived)
	Derived *to() const {
		assert(type_is<Derived>());

		auto base_ptr = get();
		return base_ptr ? static_cast<Derived *>(base_ptr) : nullptr;
	}

	_RUA_INTERFACE_CONCEPT(Derived)
	std::shared_ptr<Derived> to_shared() const {
		assert(type_is<Derived>());

		auto base_ptr = get_shared();
		return base_ptr ? std::static_pointer_cast<Derived>(base_ptr) : std::shared_ptr<Derived>();
	}

	void reset() {
		if (_base_shared_ptr) {
			_base_shared_ptr.reset();
		}
		_base_raw_ptr = nullptr;
		_derived_type = type_id<std::nullptr_t>();
	}

private:
	Base *_base_raw_ptr;
	std::shared_ptr<Base> _base_shared_ptr;
	uintptr_t _derived_type;
};

#undef _RUA_INTERFACE_CONCEPT
#undef _RUA_INTERFACE_CONCEPT_2

}

#endif
