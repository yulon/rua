#ifndef _TMD_SHARED_OBJ_HPP
#define _TMD_SHARED_OBJ_HPP

#include <memory>
#include <typeindex>
#include <type_traits>

namespace tmd {
	template <typename T>
	class dynamic_shared_obj;

	template <typename T>
	class shared_obj {
		protected:
			constexpr shared_obj() {}

		public:
			template <typename... A>
			void alloc(A&&... a) {
				_m = std::make_shared<T>(std::forward<A>(a)...);
			}

			void free() {
				_m.reset();
			}

			size_t ref_count() {
				return _m.use_count();
			}

			bool operator==(const shared_obj<T> &src) const {
				return _m.get() == src._m.get();
			}

			bool operator==(const T *mp) const {
				return _m.get() == mp;
			}

			bool operator!=(const T *mp) const {
				return _m.get() != mp;
			}

			bool operator==(std::nullptr_t) const {
				return _m.get() == nullptr;
			}

			bool operator!=(std::nullptr_t) const {
				return _m.get() != nullptr;
			}

			operator bool() const {
				return _m.get();
			}

			T *operator->() {
				return _m.get();
			}

			const T *operator->() const {
				return _m.get();
			}

			T &operator*() {
				return *_m;
			}

			const T &operator*() const {
				return *_m;
			}

			typedef T access_type;

			template <typename D>
			D cast_with_type() const {
				static_assert(std::is_base_of<dynamic_shared_obj<typename D::access_type>, D>::value, "dest is not a 'tmd::dynamic_shared_obj'");

				D dest;
				if (_m.get()) {
					static_cast<dynamic_shared_obj<typename D::access_type> &>(dest) = typeid(access_type);
					static_cast<shared_obj<typename D::access_type> &>(dest) = std::static_pointer_cast<typename D::access_type>(_m);
				} else {
					static_cast<dynamic_shared_obj<typename D::access_type> &>(dest) = typeid(nullptr);
				}
				return dest;
			}

			template <typename D>
			D cast_with_type_s() const {
				static_assert(std::is_base_of<dynamic_shared_obj<typename D::access_type>, D>::value, "dest is not a 'tmd::dynamic_shared_obj'");

				D dest;
				if (_m.get()) {
					static_cast<dynamic_shared_obj<typename D::access_type> &>(dest) = typeid(access_type);
					static_cast<shared_obj<typename D::access_type> &>(dest) = std::dynamic_pointer_cast<typename D::access_type>(_m);
				} else {
					static_cast<dynamic_shared_obj<typename D::access_type> &>(dest) = typeid(nullptr);
				}
				return dest;
			}

			void operator=(std::shared_ptr<T> &&m) {
				_m = std::move(m);
			}

			template <typename D>
			D cast() const {
				static_assert(std::is_base_of<shared_obj<typename D::access_type>, D>::value, "dest is not a 'tmd::shared_obj'");
				static_assert(!std::is_base_of<dynamic_shared_obj<typename D::access_type>, D>::value, "dest cannot be a 'tmd::dynamic_shared_obj'");

				D dest;
				if (_m.get()) {
					static_cast<shared_obj<typename D::access_type> &>(dest) = std::static_pointer_cast<typename D::access_type>(_m);
				}
				return dest;
			}

			template <typename D>
			D cast_s() const {
				static_assert(std::is_base_of<shared_obj<typename D::access_type>, D>::value, "dest is not a 'tmd::shared_obj'");
				static_assert(!std::is_base_of<dynamic_shared_obj<typename D::access_type>, D>::value, "dest cannot be a 'tmd::dynamic_shared_obj'");

				D dest;
				if (_m.get()) {
					static_cast<shared_obj<typename D::access_type> &>(dest) = std::dynamic_pointer_cast<typename D::access_type>(_m);
				}
				return dest;
			}

		protected:
			std::shared_ptr<T> _m;
	};

	template <typename T>
	class dynamic_shared_obj : public shared_obj<T> {
		protected:
			constexpr dynamic_shared_obj() : _t(typeid(nullptr)) {}

		public:
			template <typename... A>
			void alloc(A&&... a) {
				_t = typeid(T);
				shared_obj<T>::alloc(std::forward<A>(a)...);
			}

			void free() {
				_t = typeid(nullptr);
				shared_obj<T>::free();
			}

			template <typename D>
			bool type_is() {
				static_assert(std::is_base_of<shared_obj<typename D::access_type>, D>::value, "target is not a 'tmd::shared_obj'");

				return _t == typeid(typename D::access_type);
			}

			void operator=(std::type_index t) {
				_t = t;
			}

			template <typename D>
			D cast_with_type() const {
				static_assert(std::is_base_of<dynamic_shared_obj<typename D::access_type>, D>::value, "dest is not a 'tmd::dynamic_shared_obj'");

				D dest;
				static_cast<dynamic_shared_obj<typename D::access_type> &>(dest) = _t;
				if (shared_obj<T>::_m.get()) {
					static_cast<shared_obj<typename D::access_type> &>(dest) = std::static_pointer_cast<typename D::access_type>(shared_obj<T>::_m);
				}
				return dest;
			}

			template <typename D>
			D cast_with_type_s() const {
				static_assert(std::is_base_of<dynamic_shared_obj<typename D::access_type>, D>::value, "dest is not a 'tmd::dynamic_shared_obj'");

				D dest;
				static_cast<dynamic_shared_obj<typename D::access_type> &>(dest) = _t;
				if (shared_obj<T>::_m.get()) {
					static_cast<shared_obj<typename D::access_type> &>(dest) = std::dynamic_pointer_cast<typename D::access_type>(shared_obj<T>::_m);
				}
				return dest;
			}

		private:
			std::type_index _t;
	};
}

#endif
