#ifndef _RUA_REF_HPP
#define _RUA_REF_HPP

#include "macros.hpp"
#include "type_traits/macros.hpp"
#include "type_traits/type_info.hpp"

#include <cassert>
#include <cstdint>
#include <memory>

namespace rua {

template <typename T>
class ref {
public:
	constexpr ref(std::nullptr_t = nullptr) :
		_raw_ptr(nullptr),
		_shared_ptr(),
		_type(type_id<std::nullptr_t>()) {}

	template <RUA_IS_BASE_OF_CONCEPT(T, SameBase)>
	ref(SameBase *raw_ptr) {
		if (!raw_ptr) {
			_raw_ptr = nullptr;
			_type = type_id<std::nullptr_t>();
			return;
		}
		_type = type_id<SameBase>();
		_raw_ptr = static_cast<T *>(raw_ptr);
	}

	template <RUA_IS_BASE_OF_CONCEPT(T, SameBase)>
	ref(SameBase &lv) : ref(&lv) {}

	template <RUA_IS_BASE_OF_CONCEPT(T, SameBase)>
	ref(SameBase &&rv) {
		_type = type_id<SameBase>();
		new (&_shared_ptr) std::shared_ptr<T>(std::static_pointer_cast<T>(
			std::make_shared<SameBase>(std::move(rv))));
		_raw_ptr = _shared_ptr.get();
	}

	ref(const std::shared_ptr<T> &base_shared_ptr_lv) {
		if (!base_shared_ptr_lv) {
			_raw_ptr = nullptr;
			_type = type_id<std::nullptr_t>();
			return;
		}
		_type = type_id<T>();
		new (&_shared_ptr) std::shared_ptr<T>(base_shared_ptr_lv);
		_raw_ptr = _shared_ptr.get();
	}

	ref(std::shared_ptr<T> &&base_shared_ptr_rv) {
		if (!base_shared_ptr_rv) {
			_raw_ptr = nullptr;
			_type = type_id<std::nullptr_t>();
			return;
		}
		_type = type_id<T>();
		new (&_shared_ptr) std::shared_ptr<T>(std::move(base_shared_ptr_rv));
		_raw_ptr = _shared_ptr.get();
	}

	template <RUA_DERIVED_CONCEPT(T, Derived)>
	ref(const std::shared_ptr<Derived> &derived_shared_ptr_lv) {
		if (!derived_shared_ptr_lv) {
			_raw_ptr = nullptr;
			_type = type_id<std::nullptr_t>();
			return;
		}
		_type = type_id<Derived>();
		new (&_shared_ptr) std::shared_ptr<T>(
			std::static_pointer_cast<T>(derived_shared_ptr_lv));
		_raw_ptr = _shared_ptr.get();
	}

	template <RUA_DERIVED_CONCEPT(T, Derived)>
	ref(std::shared_ptr<Derived> &&derived_shared_ptr_rv) {
		if (!derived_shared_ptr_rv) {
			_raw_ptr = nullptr;
			_type = type_id<std::nullptr_t>();
			return;
		}
		_type = type_id<Derived>();
		new (&_shared_ptr) std::shared_ptr<T>(
			std::static_pointer_cast<T>(std::move(derived_shared_ptr_rv)));
		_raw_ptr = _shared_ptr.get();
	}

	template <RUA_IS_BASE_OF_CONCEPT(T, SameBase)>
	ref(const std::unique_ptr<SameBase> &unique_ptr_lv) :
		ref(unique_ptr_lv.get()) {}

	template <RUA_IS_BASE_OF_CONCEPT(T, SameBase)>
	ref(std::unique_ptr<SameBase> &&unique_ptr_rv) {
		if (!unique_ptr_rv) {
			_raw_ptr = nullptr;
			_type = type_id<std::nullptr_t>();
			return;
		}
		_type = type_id<SameBase>();
		new (&_shared_ptr)
			std::shared_ptr<T>(static_cast<T *>(unique_ptr_rv.release()));
		_raw_ptr = _shared_ptr.get();
	}

	template <RUA_DERIVED_CONCEPT(T, Derived)>
	ref(const ref<Derived> &derived_blended_ptr_lv) {
		_type = derived_blended_ptr_lv.type();

		if (!derived_blended_ptr_lv) {
			_raw_ptr = nullptr;
			return;
		}

		auto src_base_ptr_shared_ptr = derived_blended_ptr_lv.get_shared();
		if (src_base_ptr_shared_ptr) {
			new (&_shared_ptr) std::shared_ptr<T>(std::static_pointer_cast<T>(
				std::move(src_base_ptr_shared_ptr)));
			_raw_ptr = _shared_ptr.get();
			return;
		}

		_raw_ptr = static_cast<T *>(derived_blended_ptr_lv.get());
	}

	template <RUA_DERIVED_CONCEPT(T, Derived)>
	ref(ref<Derived> &&derived_blended_ptr_rv) {
		_type = derived_blended_ptr_rv.type();

		if (!derived_blended_ptr_rv) {
			_raw_ptr = nullptr;
			return;
		}

		auto src_base_ptr_shared_ptr = derived_blended_ptr_rv.release_shared();
		if (src_base_ptr_shared_ptr) {
			new (&_shared_ptr) std::shared_ptr<T>(std::static_pointer_cast<T>(
				std::move(src_base_ptr_shared_ptr)));
			_raw_ptr = _shared_ptr.get();
			return;
		}

		_raw_ptr = static_cast<T *>(derived_blended_ptr_rv.release());
	}

	ref(const ref<T> &src) {
		_raw_ptr = src._raw_ptr;
		_type = src._type;

		if (!_raw_ptr || !src._shared_ptr) {
			return;
		}

		_shared_ptr = src._shared_ptr;
	}

	ref<T> &operator=(const ref<T> &src) {
		reset();
		new (this) ref<T>(src);
		return *this;
	}

	ref(ref<T> &&src) {
		_type = src._type;

		_raw_ptr = src.release();
		if (_raw_ptr || !src) {
			return;
		}

		_shared_ptr = src.release_shared();
		_raw_ptr = _shared_ptr.get();
	}

	ref<T> &operator=(ref<T> &&src) {
		reset();
		new (this) ref<T>(std::move(src));
		return *this;
	}

	~ref() {
		reset();
	}

	operator bool() const {
		return _raw_ptr;
	}

	bool operator==(const ref<T> &src) const {
		return _raw_ptr == src._raw_ptr;
	}

	bool operator!=(const ref<T> &src) const {
		return _raw_ptr != src._raw_ptr;
	}

	template <
		typename = typename std::enable_if<
			std::is_class<T>::value || std::is_union<T>::value>::type>
	T *operator->() const {
		assert(_raw_ptr);
		return _raw_ptr;
	}

	T &operator*() const {
		return *_raw_ptr;
	}

	type_id_t type() const {
		return _type;
	}

	template <RUA_IS_BASE_OF_CONCEPT(T, SameBase)>
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

	template <RUA_IS_BASE_OF_CONCEPT(T, SameBase)>
	SameBase *as() const {
		assert(_raw_ptr);

		if (!type_is<SameBase>()) {
			return nullptr;
		}
		if (!_raw_ptr) {
			return nullptr;
		}
		return static_cast<SameBase *>(_raw_ptr);
	}

	template <RUA_IS_BASE_OF_CONCEPT(T, SameBase)>
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
	type_id_t _type;
};

} // namespace rua

#endif
