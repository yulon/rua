#ifndef _TMD_UNSAFE_PTR_HPP
#define _TMD_UNSAFE_PTR_HPP

#include "predef.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <cstring>

namespace tmd {
	class unsafe_ptr {
		public:
			constexpr unsafe_ptr() : _val(0) {}

			template <typename T>
			constexpr unsafe_ptr(T *src) : _val(reinterpret_cast<uintptr_t>(src)) {}

			constexpr unsafe_ptr(uintptr_t src) : _val(src) {}

			constexpr unsafe_ptr(std::nullptr_t) : unsafe_ptr() {}

			template <typename T>
			TMD_CONSTEXPR_14 unsafe_ptr(T &&src) : _val(
				_t2u<
					std::is_integral<T>::value ||
					(std::is_class<T>::value && std::is_convertible<T, uintptr_t>::value)
				>::fn(std::forward<T>(src)
			)) {}

			template <typename T>
			T to() const {
				return _u2t<
					T,
					std::is_pointer<T>::value,
					std::is_integral<T>::value ||
					(std::is_class<T>::value && std::is_convertible<uintptr_t, T>::value)
				>::fn(_val);
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

			uintptr_t value() const {
				return _val;
			}

			operator uintptr_t() const {
				return _val;
			}

			operator bool() const {
				return _val;
			}

			template <typename T>
			operator T *() const {
				return reinterpret_cast<T *>(_val);
			}

			template <typename T>
			operator T() const {
				return to<T>();
			}

		private:
			uintptr_t _val;

			template <bool IS_DRCT>
			struct _t2u;

			template <typename T, bool IS_PTR, bool IS_DRCT>
			struct _u2t;
	};

	template <>
	struct unsafe_ptr::_t2u<true> {
		template <typename T>
		static constexpr uintptr_t fn(T &&src) {
			return src;
		}
	};

	template <>
	struct unsafe_ptr::_t2u<false> {
		template <typename T>
		static TMD_CONSTEXPR_14 uintptr_t fn(T &&src) {
			using src_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

			if TMD_CONSTEXPR_IF (sizeof(src_t) < sizeof(uintptr_t)) {
				uintptr_t _val = 0;
				*reinterpret_cast<src_t *>(&_val) = src;
				return _val;
			} else {
				return *reinterpret_cast<const uintptr_t *>(&src);
			}
		}
	};

	template <typename T>
	struct unsafe_ptr::_u2t<T, true, false> {
		static constexpr T fn(const uintptr_t &val) {
			return reinterpret_cast<T>(val);
		}
	};

	template <typename T>
	struct unsafe_ptr::_u2t<T, false, true> {
		static constexpr T fn(const uintptr_t &val) {
			return val;
		}
	};

	template <typename T>
	struct unsafe_ptr::_u2t<T, false, false> {
		static TMD_CONSTEXPR_14 T fn(const uintptr_t &val) {
			using dest_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

			if TMD_CONSTEXPR_IF (sizeof(uintptr_t) < sizeof(dest_t)) {
				uint8_t r[sizeof(dest_t)];
				memset(reinterpret_cast<void *>(&r), 0, sizeof(dest_t));
				*reinterpret_cast<uintptr_t *>(&r) = val;
				return *reinterpret_cast<const dest_t *>(&r);
			} else {
				return *reinterpret_cast<const dest_t *>(&val);
			}
		}
	};

	template <typename T>
	inline unsafe_ptr operator+(unsafe_ptr a, T &&b) {
		return a.value() + unsafe_ptr(std::forward<T>(b)).value();
	}

	template <typename T>
	inline unsafe_ptr operator+(T &&a, unsafe_ptr b) {
		return unsafe_ptr(std::forward<T>(a)).value() + b.value();
	}

	template <typename T>
	inline ptrdiff_t operator-(unsafe_ptr a, T &&b) {
		return a.value() - unsafe_ptr(std::forward<T>(b)).value();
	}

	template <typename T>
	inline ptrdiff_t operator-(T &&a, unsafe_ptr b) {
		return unsafe_ptr(std::forward<T>(a)).value() - b.value();
	}
}

#endif
