#ifndef _RUA_UNSAFE_PTR_HPP
#define _RUA_UNSAFE_PTR_HPP

#include "predef.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <cstring>

namespace rua {
	class unsafe_ptr {
		public:
			unsafe_ptr() = default;

			constexpr unsafe_ptr(std::nullptr_t) : _val(0) {}

			template <typename T>
			constexpr unsafe_ptr(T *src) : _val(reinterpret_cast<uintptr_t>(src)) {}

			constexpr unsafe_ptr(uintptr_t src) : _val(src) {}

			template <typename T>
			RUA_CONSTEXPR_14 unsafe_ptr(T &&src) : _val(
				_t2u<
					std::is_integral<typename std::remove_reference<T>::type>::value ||
					(std::is_class<typename std::remove_reference<T>::type>::value && std::is_convertible<T, uintptr_t>::value)
				>::fn(std::forward<T>(src)
			)) {}

			constexpr unsafe_ptr(const unsafe_ptr &) = default;

			unsafe_ptr &operator=(const unsafe_ptr &) = default;

			constexpr unsafe_ptr(unsafe_ptr &&) = default;

			unsafe_ptr &operator=(unsafe_ptr &&) = default;

			constexpr unsafe_ptr(unsafe_ptr &o) : unsafe_ptr(const_cast<const unsafe_ptr &>(o)) {}

			uintptr_t &value() {
				return _val;
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
			T to() const {
				return _u2t<
					T,
					std::is_pointer<typename std::remove_reference<T>::type>::value ||
					std::is_member_pointer<typename std::remove_reference<T>::type>::value,
					std::is_integral<typename std::remove_reference<T>::type>::value ||
					(std::is_class<typename std::remove_reference<T>::type>::value && std::is_convertible<uintptr_t, T>::value)
				>::fn(_val);
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
		static RUA_CONSTEXPR_14 uintptr_t fn(T &&src) {
			using src_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

			RUA_STATIC_ASSERT(std::is_trivially_copyable<src_t>::value);

			if RUA_CONSTEXPR_IF (sizeof(src_t) < sizeof(uintptr_t)) {
				uintptr_t val = 0;
				*reinterpret_cast<src_t *>(&val) = src;
				return val;
			} else {
				return *reinterpret_cast<const uintptr_t *>(&src);
			}
		}
	};

	template <typename T, bool IS_DRCT>
	struct unsafe_ptr::_u2t<T, true, IS_DRCT> {
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
		static RUA_CONSTEXPR_14 T fn(const uintptr_t &val) {
			using dest_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

			RUA_STATIC_ASSERT(std::is_trivially_copyable<dest_t>::value);

			if RUA_CONSTEXPR_IF (sizeof(uintptr_t) < sizeof(dest_t)) {
				uint8_t r[sizeof(dest_t)];
				memset(reinterpret_cast<void *>(&r), 0, sizeof(dest_t));
				*reinterpret_cast<uintptr_t *>(&r) = val;
				return *reinterpret_cast<const dest_t *>(&r);
			} else {
				return *reinterpret_cast<const dest_t *>(&val);
			}
		}
	};
}

#endif
