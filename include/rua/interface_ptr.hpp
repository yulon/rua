#ifndef _RUA_INTERFACE_HPP
#define _RUA_INTERFACE_HPP

#include "macros.hpp"

#include <cstdint>
#include <memory>
#include <typeindex>
#include <type_traits>
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
		_base_raw_ptr(nullptr), _base_shared_ptr(), _impl_type(&typeid(std::nullptr_t))
	{}

	_RUA_INTERFACE_CONCEPT(Impl)
	interface_ptr(Impl *impl_ptr) {
		if (!impl_ptr) {
			_base_raw_ptr = nullptr;
			_impl_type = &typeid(std::nullptr_t);
			return;
		}
		_impl_type = &typeid(Impl);
		_base_raw_ptr = static_cast<Base *>(impl_ptr);
	}

	_RUA_INTERFACE_CONCEPT(Impl)
	interface_ptr(Impl &impl) : interface_ptr(&impl) {}

	_RUA_INTERFACE_CONCEPT(Impl)
	interface_ptr(Impl &&impl) {
		_impl_type = &typeid(Impl);
		new (&_base_shared_ptr) std::shared_ptr<Base>(
			std::static_pointer_cast<Base>(
				std::make_shared<Impl>(std::move(impl))
			)
		);
		_base_raw_ptr = _base_shared_ptr.get();
	}

	interface_ptr(const std::shared_ptr<Base> &impl_shared_ptr) {
		if (!impl_shared_ptr) {
			_base_raw_ptr = nullptr;
			_impl_type = &typeid(std::nullptr_t);
			return;
		}
		_impl_type = &typeid(Base);
		new (&_base_shared_ptr) std::shared_ptr<Base>(impl_shared_ptr);
		_base_raw_ptr = _base_shared_ptr.get();
	}

	interface_ptr(std::shared_ptr<Base> &&impl_shared_ptr) {
		if (!impl_shared_ptr) {
			_base_raw_ptr = nullptr;
			_impl_type = &typeid(std::nullptr_t);
			return;
		}
		_impl_type = &typeid(Base);
		new (&_base_shared_ptr) std::shared_ptr<Base>(std::move(impl_shared_ptr));
		_base_raw_ptr = _base_shared_ptr.get();
	}

	_RUA_INTERFACE_CONCEPT_2(Impl, (!std::is_same<Impl, Base>::value))
	interface_ptr(const std::shared_ptr<Impl> &impl_shared_ptr) : interface_ptr(impl_shared_ptr ? std::static_pointer_cast<Base>(impl_shared_ptr) : std::shared_ptr<Base>()) {}

	_RUA_INTERFACE_CONCEPT_2(Impl, (!std::is_same<Impl, Base>::value))
	interface_ptr(std::shared_ptr<Impl> &&impl_shared_ptr) : interface_ptr(impl_shared_ptr ? std::static_pointer_cast<Base>(std::move(impl_shared_ptr)) : std::shared_ptr<Base>()) {}

	_RUA_INTERFACE_CONCEPT(Impl)
	interface_ptr(const std::unique_ptr<Impl> &impl_unique_ptr) : interface_ptr(impl_unique_ptr.get()) {}

	_RUA_INTERFACE_CONCEPT(Impl)
	interface_ptr(std::unique_ptr<Impl> &&impl_unique_ptr) : interface_ptr(impl_unique_ptr ? std::shared_ptr<Base>(static_cast<Base *>(impl_unique_ptr.release())) : std::shared_ptr<Base>()) {}

	_RUA_INTERFACE_CONCEPT_2(OthrAbst, (!std::is_same<OthrAbst, Base>::value))
	interface_ptr(const interface_ptr<OthrAbst> &src) {
		_impl_type = src.type();

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
		_impl_type = src.type();

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
		_impl_type = src._impl_type;

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
		_impl_type = src._impl_type;

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

	const std::type_info &type() const {
		return *_impl_type;
	}

	_RUA_INTERFACE_CONCEPT(Impl)
	bool type_is() const {
		return type() == typeid(Impl);
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
		_impl_type = &typeid(_base_shared_ptr);
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
		_impl_type = &typeid(_base_shared_ptr);
		return std::move(_base_shared_ptr);
	}

	std::shared_ptr<Base> get_shared() && {
		return release_shared();
	}

	_RUA_INTERFACE_CONCEPT(Impl)
	Impl *to() const {
		assert(type_is<Impl>());

		auto base_ptr = get();
		return base_ptr ? dynamic_cast<Impl *>(base_ptr) : nullptr;
	}

	_RUA_INTERFACE_CONCEPT(Impl)
	std::shared_ptr<Impl> to_shared() const {
		assert(type_is<Impl>());

		auto base_ptr = get_shared();
		return base_ptr ? std::dynamic_pointer_cast<Impl>(base_ptr) : std::shared_ptr<Impl>();
	}

	void reset() {
		if (_base_shared_ptr) {
			_base_shared_ptr.reset();
		}
		_base_raw_ptr = nullptr;
		_impl_type = &typeid(std::nullptr_t);
	}

private:
	Base *_base_raw_ptr;
	std::shared_ptr<Base> _base_shared_ptr;
	const std::type_info *_impl_type;
};

#undef _RUA_INTERFACE_CONCEPT
#undef _RUA_INTERFACE_CONCEPT_2

}

#endif
