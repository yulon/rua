#ifndef _RUA_OBJ_HPP
#define _RUA_OBJ_HPP

#include "macros.hpp"

#include <memory>
#include <typeindex>
#include <type_traits>
#include <initializer_list>
#include <cassert>

namespace rua {
	template <typename T>
	class obj;

	template <typename T>
	constexpr bool is_obj() {
		using TT = typename std::decay<T>::type;

		return std::is_base_of<obj<typename TT::access_type>, TT>::value;
	}

	template <typename T>
	class itf;

	template <typename T>
	constexpr bool is_itf() {
		using TT = typename std::decay<T>::type;

		return std::is_base_of<itf<typename TT::access_type>, TT>::value;
	}

	template <typename T>
	class obj {
		public:
			using access_type = T;

			constexpr obj(std::nullptr_t) : _sp(nullptr) {}

			template <typename... A, typename = typename std::enable_if<std::is_constructible<T, A...>::value>::type>
			obj(A&&... a) : _sp(std::make_shared<T>(std::forward<A>(a)...)) {}

			template <typename U, typename... A, typename = typename std::enable_if<std::is_constructible<T, std::initializer_list<U>, A...>::value>::type>
			obj(std::initializer_list<U> il, A&&... a) : _sp(std::make_shared<T>(il, std::forward<A>(a)...)) {}

			void reset() {
				_sp.reset();
			}

			size_t ref_count() {
				return _sp.use_count();
			}

			bool operator==(const obj<T> &src) const {
				return _sp.get() == src._sp.get();
			}

			bool operator==(const T *mp) const {
				return _sp.get() == mp;
			}

			bool operator!=(const T *mp) const {
				return _sp.get() != mp;
			}

			bool operator==(std::nullptr_t) const {
				return _sp.get() == nullptr;
			}

			bool operator!=(std::nullptr_t) const {
				return _sp.get() != nullptr;
			}

			operator bool() const {
				return _sp.get();
			}

			T *operator->() {
				return _sp.get();
			}

			const T *operator->() const {
				return _sp.get();
			}

			std::shared_ptr<T> &operator*() {
				return _sp;
			}

			const std::shared_ptr<T> &operator*() const {
				return _sp;
			}

		private:
			std::shared_ptr<T> _sp;

			template <typename A>
			static std::shared_ptr<T> _get_shared(A &&a, std::false_type) {
				return std::make_shared<T>(std::forward<A>(a));
			}

			static std::shared_ptr<T> _get_shared(const obj<T> &o, std::true_type) {
				return o._sp;
			}

		protected:
			obj(std::shared_ptr<T> &&sp) : _sp(std::move(sp)) {}
	};

	template <bool IS_ITF>
	struct _obj_type;

	template <typename D_ELEM, bool S_IS_POLY>
	struct _ptr_cast;

	template <typename T>
	class itf : public obj<T> {
		public:
			constexpr itf() : obj<T>(nullptr), _t(typeid(nullptr)) {}
			constexpr itf(std::nullptr_t) : itf() {}

			template <typename A>
			itf(A&& a) :
				obj<T>(_get_shared(std::forward<A>(a), std::is_base_of<obj<T>, typename std::decay<A>::type>())),
				_t(_obj_type<is_itf<A>()>::fn(std::forward<A>(a)))
			{}

			void reset() {
				_t = typeid(nullptr);
				obj<T>::reset();
			}

			template <typename R>
			bool type_is() const {
				RUA_STATIC_ASSERT(is_obj<R>());

				return _t == typeid(typename std::decay<R>::type);
			}

			std::type_index type() const {
				return _t;
			}

			template <typename R>
			R to() const {
				using RR = typename std::decay<R>::type;

				RUA_STATIC_ASSERT(is_obj<R>());
				RUA_STATIC_ASSERT(!is_itf<R>());

				assert(type_is<R>());

				RR r(nullptr);
				if (*this) {
					*static_cast<obj<typename RR::access_type> &>(r) = _ptr_cast<typename RR::access_type, std::is_polymorphic<T>::value>::fn(**this);
				}
				return r;
			}

			const std::shared_ptr<T> &operator*() const {
				return obj<T>::operator*();
			}

		private:
			std::type_index _t;

			template <typename A>
			static std::shared_ptr<T> _get_shared(A &&a, std::false_type) {
				RUA_STATIC_ASSERT(is_obj<A>());

				if (a) {
					return std::static_pointer_cast<T>(*std::forward<A>(a));
				} else {
					return nullptr;
				}
			}

			static std::shared_ptr<T> _get_shared(const itf<T> &i, std::true_type) {
				if (i) {
					return *i;
				} else {
					return nullptr;
				}
			}
	};

	template <>
	struct _obj_type<false> {
		template <typename A>
		static std::type_index fn(A &&) {
			return typeid(typename std::decay<A>::type);
		}
	};

	template <>
	struct _obj_type<true> {
		template <typename A>
		static std::type_index fn(A &&a) {
			return a.type();
		}
	};

	template <typename D_ELEM>
	struct _ptr_cast<D_ELEM, false> {
		template <typename S>
		static std::shared_ptr<D_ELEM> fn(const S &src) {
			return std::static_pointer_cast<D_ELEM>(src);
		}
	};

	template <typename D_ELEM>
	struct _ptr_cast<D_ELEM, true> {
		template <typename S>
		static std::shared_ptr<D_ELEM> fn(const S &src) {
			return std::dynamic_pointer_cast<D_ELEM>(src);
		}
	};
}

#endif
