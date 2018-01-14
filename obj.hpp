#ifndef _RUA_OBJ_HPP
#define _RUA_OBJ_HPP

#include "predef.hpp"

#include <memory>
#include <typeindex>
#include <type_traits>
#include <cassert>

namespace rua {
	template <typename T>
	class obj;

	template <typename T>
	constexpr bool is_obj() {
		using TT = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

		return std::is_base_of<obj<typename TT::access_type>, TT>::value;
	}

	template <typename T>
	class itf;

	template <typename T>
	constexpr bool is_itf() {
		using TT = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

		return std::is_base_of<itf<typename TT::access_type>, TT>::value;
	}

	template <typename T>
	class obj {
		public:
			using access_type = T;

			constexpr obj(std::nullptr_t) : _sp(nullptr) {}

			template <typename... A>
			obj(A&&... a) : _sp(std::make_shared<T>(std::forward<A>(a)...)) {}

			obj(const obj &) = default;

			obj &operator=(const obj &) = default;

			obj(obj &&) = default;

			obj &operator=(obj &&) = default;

			obj(obj &o) : obj(const_cast<const obj &>(o)) {}

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
				obj<T>(nullptr),
				_t(_obj_type<is_itf<A>()>::fn(std::forward<A>(a)))
			{
				RUA_STATIC_ASSERT(is_obj<A>());

				assert(a);

				*static_cast<obj<T> &>(*this) = std::static_pointer_cast<T>(*a);
			}

			itf(const itf &) = default;

			itf &operator=(const itf &) = default;

			itf(itf &&) = default;

			itf &operator=(itf &&) = default;

			itf(itf &o) : itf(const_cast<const itf &>(o)) {}

			void reset() {
				_t = typeid(nullptr);
				obj<T>::reset();
			}

			template <typename R>
			bool type_is() const {
				RUA_STATIC_ASSERT(is_obj<R>());

				return _t == typeid(typename std::remove_cv<typename std::remove_reference<R>::type>::type);
			}

			std::type_index type() const {
				return _t;
			}

			template <typename R>
			R to() const {
				using RR = typename std::remove_cv<typename std::remove_reference<R>::type>::type;

				RUA_STATIC_ASSERT(is_obj<R>());
				RUA_STATIC_ASSERT(!is_itf<R>());

				assert(*this);
				assert(type_is<R>());

				RR r(nullptr);
				*static_cast<obj<typename RR::access_type> &>(r) = _ptr_cast<typename RR::access_type, std::is_polymorphic<T>::value>::fn(**this);
				return r;
			}

			const std::shared_ptr<T> &operator*() const {
				return obj<T>::operator*();
			}

		private:
			std::type_index _t;
	};

	template <>
	struct _obj_type<false> {
		template <typename A>
		static std::type_index fn(A &&) {
			return typeid(typename std::remove_cv<typename std::remove_reference<A>::type>::type);
		}
	};

	template <>
	struct _obj_type<true> {
		template <typename A>
		static std::type_index fn(A &&a) {
			return typeid(a.type());
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
