#ifndef _RUA_INTERFACE_PTR_HPP
#define _RUA_INTERFACE_PTR_HPP

#include "semiauto_ptr.hpp"

namespace rua {

template <typename Base>
class interface_ptr : public semiauto_ptr<Base> {
public:
	constexpr interface_ptr(std::nullptr_t = nullptr) :
		semiauto_ptr<Base>(nullptr),
		_type(type_id<std::nullptr_t>())
	{}

	RUA_IS_BASE_OF_CONCEPT(Base, Derived)
	interface_ptr(Derived *derived_ptr) :
		semiauto_ptr<Base>(derived_ptr),
		_type(derived_ptr ? type_id<Derived>() : type_id<std::nullptr_t>())
	{}

	RUA_IS_BASE_OF_CONCEPT(Base, Derived)
	interface_ptr(Derived &derived) : interface_ptr(&derived) {}

	RUA_IS_BASE_OF_CONCEPT(Base, Derived)
	interface_ptr(Derived &&derived) :
		semiauto_ptr<Base>(std::move(derived)),
		_type(type_id<Derived>())
	{}

	interface_ptr(const std::shared_ptr<Base> &base_shared_ptr) :
		semiauto_ptr<Base>(base_shared_ptr),
		_type(base_shared_ptr ? type_id<Base>() : type_id<std::nullptr_t>())
	{}

	interface_ptr(std::shared_ptr<Base> &&base_shared_ptr) :
		semiauto_ptr<Base>(std::move(base_shared_ptr)),
		_type(base_shared_ptr ? type_id<Base>() : type_id<std::nullptr_t>())
	{}

	RUA_DERIVAED_CONCEPT(Base, Derived)
	interface_ptr(const std::shared_ptr<Derived> &derived_shared_ptr) :
		semiauto_ptr<Base>(derived_shared_ptr),
		_type(derived_shared_ptr ? type_id<Derived>() : type_id<std::nullptr_t>())
	{}

	RUA_DERIVAED_CONCEPT(Base, Derived)
	interface_ptr(std::shared_ptr<Derived> &&derived_shared_ptr) :
		semiauto_ptr<Base>(std::move(derived_shared_ptr)),
		_type(derived_shared_ptr ? type_id<Derived>() : type_id<std::nullptr_t>())
	{}

	RUA_IS_BASE_OF_CONCEPT(Base, Derived)
	interface_ptr(const std::unique_ptr<Derived> &derived_unique_ptr) : interface_ptr(derived_unique_ptr.get()) {}

	RUA_IS_BASE_OF_CONCEPT(Base, Derived)
	interface_ptr(std::unique_ptr<Derived> &&derived_unique_ptr) :
		semiauto_ptr<Base>(std::move(derived_unique_ptr)),
		_type(derived_unique_ptr ? type_id<Derived>() : type_id<std::nullptr_t>())
	{}

	RUA_DERIVAED_CONCEPT(Base, Derived)
	interface_ptr(const semiauto_ptr<Derived> &src) :
		semiauto_ptr<Base>(src),
		_type(src.type())
	{}

	RUA_DERIVAED_CONCEPT(Base, Derived)
	interface_ptr(semiauto_ptr<Derived> &&src) :
		semiauto_ptr<Base>(std::move(src)),
		_type(src.type())
	{}

	RUA_DERIVAED_CONCEPT(Base, Derived)
	interface_ptr(const interface_ptr<Derived> &src) :
		semiauto_ptr<Base>(src),
		_type(src.type())
	{}

	RUA_DERIVAED_CONCEPT(Base, Derived)
	interface_ptr(interface_ptr<Derived> &&src) :
		semiauto_ptr<Base>(std::move(src)),
		_type(src.type())
	{}

	interface_ptr(const interface_ptr<Base> &src) :
		semiauto_ptr<Base>(src),
		_type(src._type)
	{}

	interface_ptr<Base> &operator=(const interface_ptr<Base> &src) {
		reset();
		new (this) interface_ptr<Base>(src);
		return *this;
	}

	interface_ptr(interface_ptr<Base> &&src)  :
		semiauto_ptr<Base>(std::move(src)),
		_type(src._type)
	{}

	interface_ptr<Base> &operator=(interface_ptr<Base> &&src) {
		reset();
		new (this) interface_ptr<Base>(std::move(src));
		return *this;
	}

	~interface_ptr() {
		reset();
	}

	uintptr_t type() const {
		return _type;
	}

	RUA_IS_BASE_OF_CONCEPT(Base, Derived)
	bool type_is() const {
		return type() == type_id<Derived>();
	}

	Base *release() {
		if (!*this) {
			return nullptr;
		}
		_type = type_id<std::nullptr_t>();
		return semiauto_ptr<Base>::release();
	}

	std::shared_ptr<Base> release_shared() {
		if (!*this) {
			return nullptr;
		}
		_type = type_id<std::nullptr_t>();
		return semiauto_ptr<Base>::release_shared();
	}

	std::shared_ptr<Base> get_shared() && {
		return release_shared();
	}

	RUA_IS_BASE_OF_CONCEPT(Base, Derived)
	Derived *to() const {
		if (!type_is<Derived>()) {
			return nullptr;
		}
		return semiauto_ptr<Base>::template to<Derived>();
	}

	RUA_IS_BASE_OF_CONCEPT(Base, Derived)
	std::shared_ptr<Derived> to_shared() const {
		if (!type_is<Derived>()) {
			return nullptr;
		}
		return semiauto_ptr<Base>::template to_shared<Derived>();
	}

	void reset() {
		semiauto_ptr<Base>::reset();
		_type = type_id<std::nullptr_t>();
	}

private:
	uintptr_t _type;
};

}

#endif
