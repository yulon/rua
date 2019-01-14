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
	typename = typename std::enable_if<std::is_base_of<Base, _T>::value>::type \
>
#define _RUA_INTERFACE_CONCEPT_2(_T, _cond) template < \
	typename _T, \
	typename = typename std::enable_if<std::is_base_of<Base, _T>::value && _cond>::type \
>

template <typename Base>
class interface_ptr {
public:
	enum class base_ptr_type_t : uint8_t {
		null_ptr = 0,
		raw_ptr,
		shared_ptr
	};

	constexpr interface_ptr(std::nullptr_t = nullptr) :
		_base_ptr_type(base_ptr_type_t::null_ptr), _base_ptr(), _impl_type(&typeid(std::nullptr_t))
	{}

	_RUA_INTERFACE_CONCEPT(Impl)
	interface_ptr(Impl *impl_ptr) {
		if (!impl_ptr) {
			_base_ptr_type = base_ptr_type_t::null_ptr;
			_impl_type = &typeid(std::nullptr_t);
			return;
		}
		_impl_type = &typeid(Impl);
		_base_ptr_type = base_ptr_type_t::raw_ptr;
		_get() = static_cast<Base *>(impl_ptr);
	}

	_RUA_INTERFACE_CONCEPT(Impl)
	interface_ptr(Impl &impl) : interface_ptr(&impl) {}

	_RUA_INTERFACE_CONCEPT(Impl)
	interface_ptr(Impl &&impl) {
		_impl_type = &typeid(Impl);
		_base_ptr_type = base_ptr_type_t::shared_ptr;
		new (&_get_shared()) std::shared_ptr<Base>(
			std::static_pointer_cast<Base>(
				std::make_shared<Impl>(std::move(impl))
			)
		);
	}

	interface_ptr(const std::shared_ptr<Base> &impl_shared_ptr) {
		if (!impl_shared_ptr) {
			_base_ptr_type = base_ptr_type_t::null_ptr;
			_impl_type = &typeid(std::nullptr_t);
			return;
		}
		_impl_type = &typeid(Base);
		_base_ptr_type = base_ptr_type_t::shared_ptr;
		new (&_get_shared()) std::shared_ptr<Base>(impl_shared_ptr);
	}

	interface_ptr(std::shared_ptr<Base> &&impl_shared_ptr) {
		if (!impl_shared_ptr) {
			_base_ptr_type = base_ptr_type_t::null_ptr;
			_impl_type = &typeid(std::nullptr_t);
			return;
		}
		_impl_type = &typeid(Base);
		_base_ptr_type = base_ptr_type_t::shared_ptr;
		new (&_get_shared()) std::shared_ptr<Base>(std::move(impl_shared_ptr));
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
		if (!src) {
			_base_ptr_type = base_ptr_type_t::null_ptr;
			_impl_type = &typeid(std::nullptr_t);
			return;
		}
		_impl_type = src.type();

		auto src_base_ptr_shared_ptr = src.get_shared();
		if (src_base_ptr_shared_ptr) {
			_base_ptr_type = base_ptr_type_t::shared_ptr;
			new (&_get_shared()) std::shared_ptr<Base>(
				std::static_pointer_cast<Base>(std::move(src_base_ptr_shared_ptr))
			);
			return;
		}

		_base_ptr_type = base_ptr_type_t::raw_ptr;
		_get() = static_cast<Base *>(src.get());
	}

	_RUA_INTERFACE_CONCEPT_2(OthrAbst, (!std::is_same<OthrAbst, Base>::value))
	interface_ptr(interface_ptr<OthrAbst> &&src) {
		if (!src) {
			_base_ptr_type = base_ptr_type_t::null_ptr;
			_impl_type = &typeid(std::nullptr_t);
			return;
		}
		_impl_type = src.type();

		auto src_base_ptr_shared_ptr = src.release_shared();
		if (src_base_ptr_shared_ptr) {
			_base_ptr_type = base_ptr_type_t::shared_ptr;
			new (&_get_shared()) std::shared_ptr<Base>(
				std::static_pointer_cast<Base>(std::move(src_base_ptr_shared_ptr))
			);
			return;
		}

		_base_ptr_type = base_ptr_type_t::raw_ptr;
		_get() = static_cast<Base *>(src.get());
	}

	interface_ptr(const interface_ptr<Base> &src) {
		_base_ptr_type = src._base_ptr_type;
		_impl_type = src._impl_type;
		switch (_base_ptr_type) {
		case base_ptr_type_t::null_ptr:
			return;
		case base_ptr_type_t::raw_ptr:
			_get() = src._get();
			return;
		case base_ptr_type_t::shared_ptr:
			new (&_get_shared()) std::shared_ptr<Base>(src._get_shared());
			return;
		}
	}

	interface_ptr<Base> &operator=(const interface_ptr<Base> &src) {
		~interface_ptr();
		new (this) interface_ptr<Base>(src);
		return *this;
	}

	interface_ptr(interface_ptr<Base> &&src) {
		_base_ptr_type = src._base_ptr_type;
		_impl_type = src._impl_type;
		switch (_base_ptr_type) {
		case base_ptr_type_t::null_ptr:
			return;
		case base_ptr_type_t::raw_ptr:
			src._base_ptr_type = base_ptr_type_t::null_ptr;
			_get() = src._get();
			return;
		case base_ptr_type_t::shared_ptr:
			src._base_ptr_type = base_ptr_type_t::null_ptr;
			new (&_get_shared()) std::shared_ptr<Base>(std::move(src._get_shared()));
			return;
		}
	}

	interface_ptr<Base> &operator=(interface_ptr<Base> &&src) {
		~interface_ptr();
		new (this) interface_ptr<Base>(src);
		return *this;
	}

	~interface_ptr() {
		if (_base_ptr_type == base_ptr_type_t::shared_ptr) {
			_get_shared().~shared_ptr();
		}
		_base_ptr_type = base_ptr_type_t::null_ptr;
		_impl_type = &typeid(std::nullptr_t);
	}

	operator bool() const {
		return _base_ptr_type != base_ptr_type_t::null_ptr;
	}

	Base *operator->() const {
		return get();
	}

	const std::type_info &type() const {
		return *_impl_type;
	}

	_RUA_INTERFACE_CONCEPT(Impl)
	bool type_is() const {
		return type() == typeid(Impl);
	}

	Base *get() const {
		switch (_base_ptr_type) {
		case base_ptr_type_t::null_ptr:
			return nullptr;
		case base_ptr_type_t::raw_ptr:
			return _get();
		case base_ptr_type_t::shared_ptr:
			return _get_shared().get();
		}
		return nullptr;
	}

	std::shared_ptr<Base> get_shared() const & {
		if (_base_ptr_type == base_ptr_type_t::shared_ptr) {
			return _get_shared();
		}
		return nullptr;
	}

	std::shared_ptr<Base> release_shared() {
		if (_base_ptr_type == base_ptr_type_t::shared_ptr) {
			_base_ptr_type = base_ptr_type_t::null_ptr;
			return std::move(_get_shared());
		}
		return nullptr;
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
		~interface_ptr();
	}

private:
	base_ptr_type_t _base_ptr_type;

	struct _base_ptr_t {
		uint8_t data[sizeof(std::shared_ptr<Base>)];

		_base_ptr_t() = default;
	} _base_ptr;

	const std::type_info *_impl_type;

	Base *&_get() {
		return *reinterpret_cast<Base **>(&_base_ptr);
	}

	Base *const &_get() const {
		return *reinterpret_cast<Base *const *>(&_base_ptr);
	}

	std::shared_ptr<Base> &_get_shared() {
		return *reinterpret_cast<std::shared_ptr<Base> *>(&_base_ptr);
	}

	const std::shared_ptr<Base> &_get_shared() const {
		return *reinterpret_cast<const std::shared_ptr<Base> *>(&_base_ptr);
	}
};

#undef _RUA_INTERFACE_CONCEPT
#undef _RUA_INTERFACE_CONCEPT_2

}

#endif
