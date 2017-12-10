#ifndef _TMD_UNSAFE_PTR_HPP
#define _TMD_UNSAFE_PTR_HPP

#include "predef.hpp"

#include <cstdint>
#include <type_traits>
#include <utility>
#include <cstring>

namespace tmd {
	class unsafe_ptr {
		public:
			constexpr unsafe_ptr() : _val(0) {}

			template <typename T>
			constexpr unsafe_ptr(T *val) : _val(reinterpret_cast<uintptr_t>(val)) {}

			constexpr unsafe_ptr(uintptr_t val) : _val(val) {}

			constexpr unsafe_ptr(std::nullptr_t) : unsafe_ptr() {}

			template <typename T>
			constexpr unsafe_ptr(T &&val) {
				if TMD_CONSTEXPR17 (sizeof(typename std::remove_cv<typename std::remove_reference<T>::type>::type) < sizeof(uintptr_t)) {
					_val = 0;
					*reinterpret_cast<typename std::remove_cv<typename std::remove_reference<T>::type>::type *>(&_val) = val;
				} else {
					_val = *reinterpret_cast<uintptr_t *>(&val);
				}
			}

			template <typename T>
			inline T to() const;

			template <typename D>
			D &get() {
				return *reinterpret_cast<D *>(_val);
			}

			template <typename D>
			const D &get() const {
				return *reinterpret_cast<D *>(_val);
			}

			template <typename D>
			D &get(size_t pos) {
				return *reinterpret_cast<D *>(_val + pos);
			}

			template <typename D>
			const D &get(size_t pos) const {
				return *reinterpret_cast<D *>(_val + pos);
			}

			template <typename D>
			D &aligned_get(size_t ix) {
				return reinterpret_cast<D *>(_val)[ix];
			}

			template <typename D>
			const D &aligned_get(size_t ix) const {
				return reinterpret_cast<D *>(_val)[ix];
			}

			uint8_t &operator[](size_t ix) {
				return get<uint8_t>(ix);
			}

			uint8_t operator[](size_t ix) const {
				return get<uint8_t>(ix);
			}

			uint8_t &operator*() {
				return get<uint8_t>();
			}

			uint8_t operator*() const {
				return get<uint8_t>();
			}

			uintptr_t value() const {
				return _val;
			}

			operator uintptr_t() const {
				return _val;
			}

			operator bool() const {
				return _val;
			}

			bool operator==(unsafe_ptr target) const {
				return _val == target._val;
			}

			bool operator!=(unsafe_ptr target) const {
				return _val != target._val;
			}

			unsafe_ptr &operator++() {
				++_val;
				return *this;
			}

			unsafe_ptr operator++(int) {
				return _val++;
			}

			unsafe_ptr &operator--() {
				--_val;
				return *this;
			}

			unsafe_ptr operator--(int) {
				return _val--;
			}

			operator uint8_t() const {
				return _val;
			}

			operator int8_t() const {
				return _val;
			}

			operator uint16_t() const {
				return _val;
			}

			operator int16_t() const {
				return _val;
			}

			operator uint32_t() const {
				return _val;
			}

			operator int32_t() const {
				return _val;
			}

			operator uint64_t() const {
				return _val;
			}

			operator int64_t() const {
				return _val;
			}

			operator void *() const {
				return reinterpret_cast<void *>(_val);
			}

			operator uint8_t *() const {
				return reinterpret_cast<uint8_t *>(_val);
			}

			operator int8_t *() const {
				return reinterpret_cast<int8_t *>(_val);
			}

			operator uint16_t *() const {
				return reinterpret_cast<uint16_t *>(_val);
			}

			operator int16_t *() const {
				return reinterpret_cast<int16_t *>(_val);
			}

			operator uint32_t *() const {
				return reinterpret_cast<uint32_t *>(_val);
			}

			operator int32_t *() const {
				return reinterpret_cast<int32_t *>(_val);
			}

			operator uint64_t *() const {
				return reinterpret_cast<uint64_t *>(_val);
			}

			operator int64_t *() const {
				return reinterpret_cast<int64_t *>(_val);
			}

			operator float() const {
				return *reinterpret_cast<const float *>(&_val);
			}

			operator double() const {
				return to<double>();
			}

			operator size_t() const {
				return _val;
			}

		private:
			uintptr_t _val;
	};

	template <typename T>
	inline T unsafe_ptr::to() const {
		if TMD_CONSTEXPR17 (sizeof(uintptr_t) > sizeof(double)) {
			typename std::remove_cv<typename std::remove_reference<T>::type>::type r;
			memset(reinterpret_cast<void *>(&r), 0, sizeof(typename std::remove_cv<typename std::remove_reference<T>::type>::type));
			*reinterpret_cast<uintptr_t *>(&r) = _val;
			return r;
		} else {
			return *reinterpret_cast<const T *>(&_val);
		}
	}

	template <>
	inline uintptr_t unsafe_ptr::to<uintptr_t>() const {
		return _val;
	}

	template <typename T>
	inline unsafe_ptr operator+(unsafe_ptr a, T &&b) {
		return a.value() + unsafe_ptr(std::forward<T>(b)).value();
	}

	template <typename T>
	inline unsafe_ptr operator+(T &&a, unsafe_ptr b) {
		return unsafe_ptr(std::forward<T>(a)).value() + b.value();
	}

	template <typename T>
	inline size_t operator-(unsafe_ptr a, T &&b) {
		return a.value() - unsafe_ptr(std::forward<T>(b)).value();
	}

	template <typename T>
	inline size_t operator-(T &&a, unsafe_ptr b) {
		return unsafe_ptr(std::forward<T>(a)).value() - b.value();
	}
}

#endif
