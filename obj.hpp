#ifndef _RUA_SHARED_OBJ_HPP
#define _RUA_SHARED_OBJ_HPP

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

			template <typename A>
			obj(A&& a) : _sp(
				_ctor<
					T,
					std::is_same<typename std::remove_cv<typename std::remove_reference<A>::type>::type, obj<T>>::value,
					std::is_same<typename std::remove_cv<typename std::remove_reference<A>::type>::type, std::shared_ptr<T>>::value
				>::fn(std::forward<A>(a))
			) {}

			obj(const obj &) = default;

			obj(obj &&) = default;

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

			template <typename TT, bool IS_THIS_T, bool IS_THIS_SP>
			struct _ctor;
	};

	template <typename T>
	template <typename TT>
	struct obj<T>::_ctor<TT, false, false> {
		template <typename A>
		static std::shared_ptr<T> fn(A &&a) {
			return std::make_shared<T>(std::forward<A>(a));
		}
	};

	template <typename T>
	template <typename TT>
	struct obj<T>::_ctor<TT, true, false> {
		static std::shared_ptr<T> fn(const obj<T> &a) {
			return a._sp;
		}
		static std::shared_ptr<T> fn(obj<T> &&a) {
			return std::move(a._sp);
		}
	};

	template <typename T>
	template <typename TT>
	struct obj<T>::_ctor<TT, false, true> {
		static std::shared_ptr<T> fn(const std::shared_ptr<T> &a) {
			return a;
		}
		static std::shared_ptr<T> fn(std::shared_ptr<T> &&a) {
			return std::move(a);
		}
	};

	template <typename T>
	class itf : public obj<T> {
		public:
			constexpr itf(std::nullptr_t) : obj<T>(nullptr), _t(typeid(nullptr)) {}

			template <typename A>
			itf(A&& a) : obj<T>(nullptr), _t(
				_get_type<
					T,
					is_itf<A>()
				>::fn(std::forward<A>(a))
			) {
				static_assert(is_obj<A>(), "dest is not a 'rua::obj'");

				assert(a);

				obj<T>::operator*() = std::static_pointer_cast<T>(*a);
			}

			void reset() {
				_t = typeid(nullptr);
				obj<T>::reset();
			}

			template <typename R>
			bool type_is() const {
				static_assert(is_obj<R>(), "target is not a 'rua::obj'");

				return _t == typeid(typename std::remove_cv<typename std::remove_reference<R>::type>::type);
			}

			template <typename R>
			R to() const {
				using RR = typename std::remove_cv<typename std::remove_reference<R>::type>::type;

				static_assert(is_obj<R>(), "dest is not a 'rua::obj'");
				static_assert(!is_itf<R>(), "dest cannot be a 'rua::itf'");

				assert(*this);
				assert(type_is<R>());

				return std::static_pointer_cast<typename RR::access_type>(**this);
			}

			std::shared_ptr<T> &operator*() = delete;

			const std::shared_ptr<T> &operator*() const {
				return obj<T>::operator*();
			}

		private:
			std::type_index _t;

			template <typename TT, bool IS_ITF>
			struct _get_type;
	};

	template <typename T>
	template <typename TT>
	struct itf<T>::_get_type<TT, false> {
		template <typename A>
		static std::type_index fn(A &&) {
			return typeid(typename std::remove_cv<typename std::remove_reference<A>::type>::type);
		}
	};

	template <typename T>
	template <typename TT>
	struct itf<T>::_get_type<TT, true> {
		template <typename A>
		static std::type_index fn(A &&a) {
			return typeid(a._t);
		}
	};
}

#endif
