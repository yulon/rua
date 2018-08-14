#ifndef _RUA_ANY_WORD_HPP
#define _RUA_ANY_WORD_HPP

#include "macros.hpp"
#include "tmp.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <cassert>

namespace rua {
	class any_word {
		public:
			any_word() = default;

			constexpr any_word(std::nullptr_t) : _val(0) {}

			template <typename T>
			RUA_CONSTEXPR_14 any_word(T &&src) : _val(_in(std::forward<T>(src))) {}

			uintptr_t &value() {
				return _val;
			}

			uintptr_t value() const {
				return _val;
			}

			template <typename T>
			T to() const {
				return _out<T>();
			}

			template <typename T>
			operator T() const {
				return to<T>();
			}

		private:
			uintptr_t _val;

			struct _is_int {};
			struct _is_sm {};

			template <typename T>
			static constexpr uintptr_t _in(T &&src, _is_int &&) {
				return static_cast<uintptr_t>(src);
			}

			template <typename T>
			static RUA_CONSTEXPR_14 uintptr_t _in(T &&src, _is_sm &&) {
				union val_u {
					typename std::decay<T>::type src_v;
					uintptr_t uint_v;
				};
				return val_u{std::forward<T>(src)}.uint_v;
			}

			template <typename T>
			static constexpr uintptr_t _in(T &&src, tmp::default_t &&) {
				return *reinterpret_cast<uintptr_t *>(&src);
			}

			template <typename T>
			static RUA_CONSTEXPR_14 uintptr_t _in(T &&src) {
				using TT = typename std::decay<T>::type;

				return _in(
					std::forward<T>(src),
					tmp::switch_true_t<
						std::is_integral<TT>,
						_is_int,
						tmp::bool_constant<sizeof(TT) < sizeof(uintptr_t)>,
						_is_sm
					>{}
				);
			}

			struct _is_big {};

			template <typename T>
			T _out(_is_int &&) const {
				return static_cast<T>(_val);
			}

			template <typename T>
			T _out(_is_big &&) const {
				uint8_t r[sizeof(T)];
				*reinterpret_cast<uintptr_t *>(&r) = _val;
				return *reinterpret_cast<T *>(&r);
			}

			template <typename T>
			T _out(tmp::default_t &&) const {
				return *reinterpret_cast<const T *>(this);
			}

			template <typename T>
			T _out() const {
				using TT = typename std::decay<T>::type;

				return _out<TT>(tmp::switch_true_t<
					std::is_integral<TT>,
					_is_int,
					tmp::bool_constant<(sizeof(TT) > sizeof(uintptr_t))>,
					_is_big
				>{});
			}
	};
}

#endif
