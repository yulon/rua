#ifndef _RUA_ANY_PTR_HPP
#define _RUA_ANY_PTR_HPP

#include "macros.hpp"
#include "type_traits.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <cassert>

namespace rua {
	class any_ptr {
		public:
			any_ptr() = default;

			RUA_FORCE_INLINE constexpr any_ptr(std::nullptr_t) : _val(0) {}

			template <typename T>
			RUA_FORCE_INLINE constexpr any_ptr(T *src) : _val(reinterpret_cast<uintptr_t>(src)) {}

			RUA_FORCE_INLINE explicit constexpr any_ptr(uintptr_t src) : _val(src) {}

			RUA_FORCE_INLINE uintptr_t &value() {
				return _val;
			}

			RUA_FORCE_INLINE uintptr_t value() const {
				return _val;
			}

			RUA_FORCE_INLINE operator bool() const {
				return _val;
			}

			template <typename T, typename = typename std::enable_if<std::is_pointer<T>::value>::type>
			RUA_FORCE_INLINE T to() const {
				return reinterpret_cast<T>(_val);
			}

			template <typename T>
			RUA_FORCE_INLINE T *point() const {
				return reinterpret_cast<T *>(_val);
			}

			template <typename T>
			RUA_FORCE_INLINE operator T *() const {
				return reinterpret_cast<T *>(_val);
			}

			RUA_FORCE_INLINE explicit operator uintptr_t() const {
				return _val;
			}

			RUA_FORCE_INLINE any_ptr &operator++() {
				++_val;
				return *this;
			}

			RUA_FORCE_INLINE any_ptr operator++() const {
				return any_ptr(_val + 1);
			}

			RUA_FORCE_INLINE any_ptr operator++(int) {
				return any_ptr(_val++);
			}

			RUA_FORCE_INLINE any_ptr &operator--() {
				--_val;
				return *this;
			}

			RUA_FORCE_INLINE any_ptr operator--() const {
				return any_ptr(_val - 1);
			}

			RUA_FORCE_INLINE any_ptr operator--(int) {
				return any_ptr(_val--);
			}

			template <typename T>
			RUA_FORCE_INLINE any_ptr operator+(T &&target) const {
				return any_ptr(_val + static_cast<uintptr_t>(target));
			}

			RUA_FORCE_INLINE ptrdiff_t operator-(const any_ptr &target) const {
				return static_cast<ptrdiff_t>(_val - target._val);
			}

			template <typename T, typename = typename std::enable_if<!std::is_convertible<typename std::decay<T>::type, any_ptr>::value>::type>
			RUA_FORCE_INLINE any_ptr operator-(T &&target) const {
				return any_ptr(_val - static_cast<uintptr_t>(target));
			}

			RUA_FORCE_INLINE bool operator>(const any_ptr &target) const {
				return _val > target._val;
			}

			RUA_FORCE_INLINE bool operator<(const any_ptr &target) const {
				return _val < target._val;
			}

			RUA_FORCE_INLINE bool operator>=(const any_ptr &target) const {
				return _val >= target._val;
			}

			RUA_FORCE_INLINE bool operator<=(const any_ptr &target) const {
				return _val <= target._val;
			}

			RUA_FORCE_INLINE bool operator==(const any_ptr &target) const {
				return _val == target._val;
			}

			template <typename T>
			RUA_FORCE_INLINE any_ptr &operator+=(T &&target) {
				_val += static_cast<uintptr_t>(target);
				return *this;
			}

			template <typename T, typename = typename std::enable_if<!std::is_convertible<typename std::decay<T>::type, any_ptr>::value>::type>
			RUA_FORCE_INLINE any_ptr &operator-=(T &&target) {
				_val -= static_cast<uintptr_t>(target);
				return *this;
			}

		private:
			uintptr_t _val;
	};
}

#endif
