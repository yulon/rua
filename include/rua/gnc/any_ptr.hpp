#ifndef _RUA_GNC_ANY_PTR_HPP
#define _RUA_GNC_ANY_PTR_HPP

#include "../macros.hpp"
#include "../tmp.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <limits>
#include <cassert>

#include "../disable_msvc_sh1t.h"

namespace rua {
	class any_ptr {
		public:
			any_ptr() = default;

			constexpr any_ptr(std::nullptr_t) : _val(0) {}

			template <typename T>
			constexpr any_ptr(T *src) : _val(reinterpret_cast<uintptr_t>(src)) {}

			explicit constexpr any_ptr(uintptr_t src) : _val(src) {}

			uintptr_t &value() {
				return _val;
			}

			uintptr_t value() const {
				return _val;
			}

			operator bool() const {
				return _val;
			}

			template <typename T>
			T to() const {
				return _out<T>();
			}

			template <typename T>
			explicit operator T() const {
				return to<T>();
			}

			template <typename T>
			operator T *() const {
				return to<T *>();
			}

			any_ptr &operator++() {
				++_val;
				return *this;
			}

			any_ptr operator++() const {
				return any_ptr(_val + 1);
			}

			any_ptr operator++(int) {
				return any_ptr(_val++);
			}

			any_ptr &operator--() {
				--_val;
				return *this;
			}

			any_ptr operator--() const {
				return any_ptr(_val - 1);
			}

			any_ptr operator--(int) {
				return any_ptr(_val--);
			}

			template <typename T>
			any_ptr operator+(T &&target) const {
				return any_ptr(_val + static_cast<uintptr_t>(target));
			}

			ptrdiff_t operator-(const any_ptr &target) const {
				return static_cast<ptrdiff_t>(_val - target._val);
			}

			template <typename T>
			any_ptr operator-(T &&target) const {
				return any_ptr(_val - static_cast<uintptr_t>(target));
			}

			bool operator>(const any_ptr &target) const {
				return _val > target._val;
			}

			bool operator<(const any_ptr &target) const {
				return _val < target._val;
			}

			bool operator>=(const any_ptr &target) const {
				return _val >= target._val;
			}

			bool operator<=(const any_ptr &target) const {
				return _val <= target._val;
			}

			bool operator==(const any_ptr &target) const {
				return _val == target._val;
			}

		private:
			uintptr_t _val;

			struct _is_num {};
			struct _is_unum {};
			struct _is_ptr {};
			struct _is_cvt_from_gp {};
			struct _is_cst_from_gp {};
			struct _is_cvt {};
			struct _is_cst {};

			template <typename T>
			T _out(_is_ptr &&) const {
				return reinterpret_cast<T>(_val);
			}

			template <typename T>
			T _out(_is_num &&) const {
				return static_cast<T>(_val);
			}

			template <typename T>
			T _out(_is_unum &&) const {
				return static_cast<T>(_val);
			}

			template <typename T>
			T _out(_is_cvt_from_gp &&) const {
				return reinterpret_cast<void *>(_val);
			}

			template <typename T>
			T _out(_is_cst_from_gp &&) const {
				return T(reinterpret_cast<void *>(_val));
			}

			template <typename T>
			T _out(_is_cvt &&) const {
				return _val;
			}

			template <typename T>
			T _out(_is_cst &&) const {
				return T(_val);
			}

			template <typename T>
			T _out() const {
				using TT = typename std::decay<T>::type;

				return _out<TT>(tmp::switch_true_t<
					tmp::bool_constant<
						std::is_pointer<TT>::value ||
						std::is_member_pointer<TT>::value
					>,
					_is_ptr,

					tmp::bool_constant<
						std::is_integral<TT>::value ||
						std::is_floating_point<TT>::value
					>,
					tmp::switch_true_t<
						std::is_signed<TT>,
						_is_num,
						_is_unum
					>,

					std::is_class<TT>,
					tmp::switch_true_t<
						std::is_convertible<void *, TT>,
						_is_cvt_from_gp,

						std::is_constructible<TT, void *>,
						_is_cst_from_gp,

						std::is_convertible<uintptr_t, TT>,
						_is_cvt,

						std::is_constructible<TT, uintptr_t>,
						_is_cst
					>
				>{});
			}
	};
}

#endif
