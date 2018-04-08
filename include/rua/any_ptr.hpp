#ifndef _RUA_ANY_PTR_HPP
#define _RUA_ANY_PTR_HPP

#include "macros.hpp"
#include "tmp.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <limits>
#include <cassert>

#include "disable_msvc_sh1t.h"

namespace rua {
	class any_ptr {
		public:
			any_ptr() = default;

			constexpr any_ptr(std::nullptr_t) : _val(0) {}

			template <typename T>
			explicit RUA_CONSTEXPR_14 any_ptr(T &&src) : _val(_in(std::forward<T>(src))) {}

			template <typename T>
			constexpr any_ptr(T *src) : _val(reinterpret_cast<uintptr_t>(src)) {}

			uintptr_t &value() {
				return _val;
			}

			uintptr_t value() const {
				return _val;
			}

			intptr_t &signed_value() {
				assert(_val <= static_cast<uintptr_t>(std::numeric_limits<intptr_t>::max()));

				return *reinterpret_cast<intptr_t *>(&_val);
			}

			intptr_t signed_value() const {
				assert(_val <= static_cast<uintptr_t>(std::numeric_limits<intptr_t>::max()));

				return static_cast<intptr_t>(_val);
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

			template <typename T>
			any_ptr operator+(T &&target) const {
				return _ptr_plus(std::forward<T>(target));
			}

			ptrdiff_t operator-(const any_ptr &target) const {
				return signed_value() - target.signed_value();
			}

			template <typename T>
			any_ptr operator-(T &&target) const {
				return _ptr_div(std::forward<T>(target));
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

			struct _is_same {};
			struct _is_num {};
			struct _is_unum {};

			template <typename T>
			static constexpr uintptr_t _in(T &&src, _is_same &&) {
				return src._val;
			}

			template <typename T>
			static RUA_CONSTEXPR_14 uintptr_t _in(T &&src, _is_num &&) {
				return src < 0 ? 0 : static_cast<uintptr_t>(src);
			}

			template <typename T>
			static constexpr uintptr_t _in(T &&src, _is_unum &&) {
				return static_cast<uintptr_t>(src);
			}

			static constexpr uintptr_t _in(uintptr_t src, tmp::default_t &&) {
				return src;
			}

			template <typename T>
			static constexpr uintptr_t _in(T *src, tmp::default_t &&) {
				return reinterpret_cast<uintptr_t>(src);
			}

			template <typename T>
			static RUA_CONSTEXPR_14 uintptr_t _in(T &&src) {
				using TT = typename std::decay<T>::type;

				return _in(
					std::forward<T>(src),
					tmp::switch_true_t<
						std::is_base_of<any_ptr, TT>,
						_is_same,
						std::is_signed<TT>,
						_is_num,
						tmp::bool_constant<
							std::is_integral<TT>::value ||
							std::is_floating_point<TT>::value
						>,
						_is_unum
					>{}
				);
			}

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
				return static_cast<T>(signed_value());
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

			template <typename T>
			any_ptr _ptr_plus(T &&target, std::false_type &&) const {
				return any_ptr(_val + static_cast<uintptr_t>(std::forward<T>(target)));
			}

			template <typename T>
			any_ptr _ptr_plus(T &&target, std::true_type &&) const {
				if (target < 0) {
					return any_ptr(_val - static_cast<uintptr_t>(0 - (std::forward<T>(target))));
				}
				return any_ptr(_val + static_cast<uintptr_t>(std::forward<T>(target)));
			}

			template <typename T>
			any_ptr _ptr_plus(T &&src) const {
				return _ptr_plus(std::forward<T>(src), std::is_signed<typename std::decay<T>::type>());
			}

			template <typename T>
			any_ptr _ptr_div(T &&target, std::false_type &&) const {
				return any_ptr(_val + static_cast<uintptr_t>(std::forward<T>(target)));
			}

			template <typename T>
			any_ptr _ptr_div(T &&target, std::true_type &&) const {
				if (target < 0) {
					return any_ptr(_val + static_cast<uintptr_t>(0 - (std::forward<T>(target))));
				}
				return any_ptr(_val - static_cast<uintptr_t>(std::forward<T>(target)));
			}

			template <typename T>
			any_ptr _ptr_div(T &&src) const {
				return _ptr_div(std::forward<T>(src), std::is_signed<typename std::decay<T>::type>());
			}
	};
}

#endif
