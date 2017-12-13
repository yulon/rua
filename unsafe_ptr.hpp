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
			constexpr unsafe_ptr(T *src) : _val(reinterpret_cast<uintptr_t>(src)) {}

			constexpr unsafe_ptr(uintptr_t val) : _val(val) {}

			constexpr unsafe_ptr(std::nullptr_t) : unsafe_ptr() {}

			template <typename T>
			TMD_CONSTEXPR_14 unsafe_ptr(T &&src) {
				using src_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

				#ifdef TMD_CONSTEXPR_IF_SUPPORTED
					if constexpr (
						std::is_integral_v<src_t> ||
						(std::is_class_v<src_t> && std::is_convertible_v<src_t, uintptr_t>)
					) {
						_val = src;
					} else
				#endif

				if (sizeof(src_t) < sizeof(uintptr_t)) {
					_val = 0;
					*reinterpret_cast<src_t *>(&_val) = src;
				} else {
					static_assert(
						sizeof(src_t) > sizeof(uintptr_t) ? !(std::is_class<src_t>::value && std::is_convertible<src_t, uintptr_t>::value) : true,
						"please use C++17!"
					);

					_val = *reinterpret_cast<const uintptr_t *>(&src);
				}
			}

			template <typename T>
			inline T to() const;

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
	};

	template <typename T>
	inline T unsafe_ptr::to() const {
		using dest_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

		#ifdef TMD_CONSTEXPR_IF_SUPPORTED
			if constexpr (
				std::is_integral_v<dest_t> ||
				(std::is_class_v<dest_t> && std::is_convertible_v<uintptr_t, dest_t>)
			) {
				return _val;
			} else
		#endif

		if (sizeof(uintptr_t) < sizeof(dest_t)) {
			static_assert(
				!(std::is_class<dest_t>::value && std::is_convertible<uintptr_t, dest_t>::value),
				"please use C++17!"
			);

			uint8_t r[sizeof(dest_t)];
			memset(reinterpret_cast<void *>(&r), 0, sizeof(dest_t));
			*reinterpret_cast<uintptr_t *>(&r) = _val;
			return *reinterpret_cast<const dest_t *>(&r);
		} else {
			return *reinterpret_cast<const dest_t *>(&_val);
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
