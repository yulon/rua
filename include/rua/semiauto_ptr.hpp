#ifndef _RUA_SEMIAUTO_PTR_HPP
#define _RUA_SEMIAUTO_PTR_HPP

#include "macros.hpp"
#include "type_traits.hpp"

#include <cstdint>
#include <memory>
#include <typeindex>
#include <cassert>

namespace rua {

template <typename T>
class semiauto_ptr {
public:
	constexpr semiauto_ptr(std::nullptr_t = nullptr) :
		_raw_ptr(nullptr),
		_shared_ptr()
	{}

	RUA_IS_BASE_OF_CONCEPT(T, SRC)
	semiauto_ptr(SRC *ptr) {
		if (!ptr) {
			_raw_ptr = nullptr;
			return;
		}
		_raw_ptr = static_cast<T *>(ptr);
	}

	RUA_IS_BASE_OF_CONCEPT(T, SRC)
	semiauto_ptr(SRC &lv) : semiauto_ptr(&lv) {}

	RUA_IS_BASE_OF_CONCEPT(T, SRC)
	semiauto_ptr(SRC &&rv) {
		new (&_shared_ptr) std::shared_ptr<T>(
			std::static_pointer_cast<T>(
				std::make_shared<SRC>(std::move(rv))
			)
		);
		_raw_ptr = _shared_ptr.get();
	}

	semiauto_ptr(const std::shared_ptr<T> &shared_ptr_lv) {
		if (!shared_ptr_lv) {
			_raw_ptr = nullptr;
			return;
		}
		new (&_shared_ptr) std::shared_ptr<T>(
			shared_ptr_lv
		);
		_raw_ptr = _shared_ptr.get();
	}

	semiauto_ptr(std::shared_ptr<T> &&shared_ptr_rv) {
		if (!shared_ptr_rv) {
			_raw_ptr = nullptr;
			return;
		}
		new (&_shared_ptr) std::shared_ptr<T>(
			std::move(shared_ptr_rv)
		);
		_raw_ptr = _shared_ptr.get();
	}

	RUA_DERIVAED_CONCEPT(T, SRC)
	semiauto_ptr(const std::shared_ptr<SRC> &derived_shared_ptr_lv) {
		if (!derived_shared_ptr_lv) {
			_raw_ptr = nullptr;
			return;
		}
		new (&_shared_ptr) std::shared_ptr<T>(
			std::static_pointer_cast<T>(derived_shared_ptr_lv)
		);
		_raw_ptr = _shared_ptr.get();
	}

	RUA_DERIVAED_CONCEPT(T, SRC)
	semiauto_ptr(std::shared_ptr<SRC> &&derived_shared_ptr_rv) {
		if (!derived_shared_ptr_rv) {
			_raw_ptr = nullptr;
			return;
		}
		new (&_shared_ptr) std::shared_ptr<T>(
			std::static_pointer_cast<T>(std::move(derived_shared_ptr_rv))
		);
		_raw_ptr = _shared_ptr.get();
	}

	RUA_IS_BASE_OF_CONCEPT(T, SRC)
	semiauto_ptr(const std::unique_ptr<SRC> &unique_ptr_lv) : semiauto_ptr(unique_ptr_lv.get()) {}

	RUA_IS_BASE_OF_CONCEPT(T, SRC)
	semiauto_ptr(std::unique_ptr<SRC> &&unique_ptr_rv) {
		if (!unique_ptr_rv) {
			_raw_ptr = nullptr;
			return;
		}
		new (&_shared_ptr) std::shared_ptr<T>(
			static_cast<T *>(unique_ptr_rv.release())
		);
		_raw_ptr = _shared_ptr.get();
	}

	RUA_DERIVAED_CONCEPT(T, SRC)
	semiauto_ptr(const semiauto_ptr<SRC> &src) {
		if (!src) {
			_raw_ptr = nullptr;
			return;
		}
		auto src_base_ptr_shared_ptr = src.get_shared();
		if (src_base_ptr_shared_ptr) {
			new (&_shared_ptr) std::shared_ptr<T>(
				std::static_pointer_cast<T>(std::move(src_base_ptr_shared_ptr))
			);
			_raw_ptr = _shared_ptr.get();
			return;
		}
		_raw_ptr = static_cast<T *>(src.get());
	}

	RUA_DERIVAED_CONCEPT(T, SRC)
	semiauto_ptr(semiauto_ptr<SRC> &&src) {
		if (!src) {
			_raw_ptr = nullptr;
			return;
		}
		auto src_base_ptr_shared_ptr = src.release_shared();
		if (src_base_ptr_shared_ptr) {
			new (&_shared_ptr) std::shared_ptr<T>(
				std::static_pointer_cast<T>(std::move(src_base_ptr_shared_ptr))
			);
			_raw_ptr = _shared_ptr.get();
			return;
		}
		_raw_ptr = static_cast<T *>(src.release());
	}

	semiauto_ptr(const semiauto_ptr<T> &src) {
		_raw_ptr = src._raw_ptr;
		if (!_raw_ptr || !src._shared_ptr) {
			return;
		}
		_shared_ptr = src._shared_ptr;
	}

	semiauto_ptr<T> &operator=(const semiauto_ptr<T> &src) {
		reset();
		new (this) semiauto_ptr<T>(src);
		return *this;
	}

	semiauto_ptr(semiauto_ptr<T> &&src) {
		_raw_ptr = src.release();
		if (_raw_ptr || !src) {
			return;
		}
		_shared_ptr = src.release_shared();
		_raw_ptr = _shared_ptr.get();
	}

	semiauto_ptr<T> &operator=(semiauto_ptr<T> &&src) {
		reset();
		new (this) semiauto_ptr<T>(std::move(src));
		return *this;
	}

	~semiauto_ptr() {
		reset();
	}

	operator bool() const {
		return _raw_ptr;
	}

	bool operator==(const semiauto_ptr<T> &src) const {
		return _raw_ptr == src._raw_ptr;
	}

	bool operator!=(const semiauto_ptr<T> &src) const {
		return _raw_ptr != src._raw_ptr;
	}

	T *operator->() const {
		assert(_raw_ptr);
		return _raw_ptr;
	}

	T &operator*() const {
		return *_raw_ptr;
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
		return std::move(_shared_ptr);
	}

	std::shared_ptr<T> get_shared() && {
		return release_shared();
	}

	RUA_IS_BASE_OF_CONCEPT(T, SRC)
	SRC *to() const {
		assert(_raw_ptr);

		if (!_raw_ptr) {
			return nullptr;
		}
		return static_cast<SRC *>(_raw_ptr);
	}

	RUA_IS_BASE_OF_CONCEPT(T, SRC)
	std::shared_ptr<SRC> to_shared() const {
		assert(_raw_ptr);

		auto base_ptr = get_shared();
		if (!base_ptr) {
			return nullptr;
		}
		return std::static_pointer_cast<SRC>(base_ptr);
	}

	void reset() {
		if (_shared_ptr) {
			_shared_ptr.reset();
		}
		_raw_ptr = nullptr;
	}

private:
	T *_raw_ptr;
	std::shared_ptr<T> _shared_ptr;
};

}

#endif
