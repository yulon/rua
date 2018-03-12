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
			RUA_CONSTEXPR_14 unsafe_ptr(T &&src) : _val(_t2u(
				std::forward<T>(src),
				std::integral_constant<bool,
					std::is_integral<typename std::remove_reference<T>::type>::value ||
					(
						std::is_class<typename std::remove_reference<T>::type>::value &&
						std::is_convertible<T, uintptr_t>::value
					)
				>()
			)) {}

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
				return _u2t<T>(
					_val,
					std::integral_constant<bool,
						std::is_pointer<typename std::remove_reference<T>::type>::value ||
						std::is_member_pointer<typename std::remove_reference<T>::type>::value
					>(),
					std::integral_constant<bool,
						std::is_integral<typename std::remove_reference<T>::type>::value ||
						(std::is_class<typename std::remove_reference<T>::type>::value && std::is_convertible<uintptr_t, T>::value)
					>()
				);
			}

			template <typename T>
			operator T() const {
				return to<T>();
			}

		private:
			uintptr_t _val;

			template <typename T>
			static constexpr uintptr_t _t2u(T &&src, std::true_type /*is_drct*/) {
				return src;
			}

			template <typename T>
			static RUA_CONSTEXPR_14 uintptr_t _t2u(T &&src, std::false_type /*is_drct*/) {
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

			template <typename T, typename IS_DRCT>
			static constexpr T _u2t(const uintptr_t &val, std::true_type /*is_ptr*/, IS_DRCT &&) {
				return reinterpret_cast<T>(val);
			}

			template <typename T>
			static constexpr T _u2t(const uintptr_t &val, std::false_type /*is_ptr*/, std::true_type /*is_drct*/) {
				return val;
			}

			template <typename T>
			static RUA_CONSTEXPR_14 T _u2t(const uintptr_t &val, std::false_type /*is_ptr*/, std::false_type /*is_drct*/) {
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
