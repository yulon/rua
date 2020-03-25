#ifndef _RUA_TYPES_INFO_HPP
#define _RUA_TYPES_INFO_HPP

#include "traits.hpp"
#include "util.hpp"

#include "../macros.hpp"

#include <cassert>
#include <string>

#ifdef RUA_RTTI
#include <typeindex>
#endif

namespace rua {

class type_info {
public:
	constexpr type_info() : _inf(nullptr) {}

	operator bool() const {
		return _inf;
	}

	bool operator!() const {
		return !_inf;
	}

	bool operator==(const type_info &target) const {
		return _inf == target._inf;
	}

	bool operator!=(const type_info &target) const {
		return _inf != target._inf;
	}

	RUA_FORCE_INLINE size_t size() const {
		return _inf ? _inf->size : 0;
	}

	RUA_FORCE_INLINE size_t align() const {
		return _inf ? _inf->size : 0;
	}

	RUA_FORCE_INLINE bool is_trivial() const {
		return _inf ? _inf->is_trivial : false;
	}

	RUA_FORCE_INLINE const std::string &desc() const {
		return _inf ? _inf->desc() : _desc<void>();
	}

	RUA_FORCE_INLINE std::string &desc() {
		return _inf ? _inf->desc() : _desc<void>();
	}

	RUA_FORCE_INLINE size_t hash_code() const {
		return static_cast<size_t>(reinterpret_cast<uintptr_t>(_inf));
	}

	RUA_FORCE_INLINE void dtor(void *ptr) const {
		assert(_inf);
		if (!_inf->dtor) {
			return;
		}
		_inf->dtor(ptr);
	}

	RUA_FORCE_INLINE void del(void *ptr) const {
		assert(_inf);
		assert(_inf->del);
		_inf->del(ptr);
	}

	RUA_FORCE_INLINE bool is_copyable() const {
		return _inf && _inf->copy_ctor;
	}

	RUA_FORCE_INLINE void copy_ctor(void *ptr, const void *src) const {
		assert(_inf);
		assert(_inf->copy_ctor);
		_inf->copy_ctor(ptr, src);
	}

	RUA_FORCE_INLINE void *copy_new(const void *src) const {
		assert(_inf);
		assert(_inf->copy_new);
		return _inf->copy_new(src);
	}

	RUA_FORCE_INLINE bool is_moveable() const {
		return _inf && _inf->move_ctor;
	}

	RUA_FORCE_INLINE void move_ctor(void *ptr, void *src) const {
		assert(_inf);
		assert(_inf->move_ctor);
		_inf->move_ctor(ptr, src);
	}

	RUA_FORCE_INLINE void *move_new(void *src) const {
		assert(_inf);
		assert(_inf->move_new);
		return _inf->move_new(src);
	}

#ifdef RUA_RTTI
	RUA_FORCE_INLINE const std::type_info &std_id() const {
		return _inf ? _inf->std_id() : typeid(void);
	}
#endif

private:
	struct _tab_t {
		const size_t size;

		const size_t align;

		const bool is_trivial;

		std::string &(*const desc)();

		void (*const dtor)(void *);

		void (*const del)(void *);

		void (*const copy_ctor)(void *ptr, const void *src);

		void *(*const copy_new)(const void *src);

		void (*const move_ctor)(void *ptr, void *src);

		void *(*const move_new)(void *src);

#ifdef RUA_RTTI
		const std::type_info &(*const std_id)();
#endif
	};

	template <typename T, typename = void>
	struct _dtor {
		static constexpr std::nullptr_t value = nullptr;
	};

	template <typename T>
	struct _dtor<
		T,
		enable_if_t<size_of<T>::value && !std::is_trivial<T>::value>> {
		static void value(void *ptr) {
			reinterpret_cast<T *>(ptr)->~T();
		}
	};

	template <typename T, typename = void>
	struct _del {
		static constexpr std::nullptr_t value = nullptr;
	};

	template <typename T>
	struct _del<T, enable_if_t<(size_of<T>::value > 0)>> {
		static void value(void *ptr) {
			delete reinterpret_cast<T *>(ptr);
		}
	};

	template <typename T, typename = void>
	struct _copy_ctor {
		static constexpr std::nullptr_t value = nullptr;
	};

	template <typename T>
	struct _copy_ctor<T, enable_if_t<std::is_copy_constructible<T>::value>> {
		static void value(void *ptr, const void *src) {
			new (reinterpret_cast<remove_cv_t<T> *>(ptr))
				T(*reinterpret_cast<const T *>(src));
		};
	};

	template <typename T, typename = void>
	struct _copy_new {
		static constexpr std::nullptr_t value = nullptr;
	};

	template <typename T>
	struct _copy_new<T, enable_if_t<std::is_copy_constructible<T>::value>> {
		static void *value(const void *src) {
			return new T(*reinterpret_cast<const T *>(src));
		}
	};

	template <typename T, typename = void>
	struct _move_ctor {
		static constexpr std::nullptr_t value = nullptr;
	};

	template <typename T>
	struct _move_ctor<T, enable_if_t<std::is_move_constructible<T>::value>> {
		static void value(void *ptr, void *src) {
			new (reinterpret_cast<remove_cv_t<T> *>(ptr))
				T(std::move(*reinterpret_cast<T *>(src)));
		}
	};

	template <typename T, typename = void>
	struct _move_new {
		static constexpr std::nullptr_t value = nullptr;
	};

	template <typename T>
	struct _move_new<T, enable_if_t<std::is_move_constructible<T>::value>> {
		static void *value(void *src) {
			return new T(std::move(*reinterpret_cast<T *>(src)));
		}
	};

	template <typename T>
	static std::string &_desc() {
		static std::string desc;
		return desc;
	}

#ifdef RUA_RTTI
	template <typename T>
	static const std::type_info &_std_id() {
		return typeid(T);
	}
#endif

	template <typename T>
	static RUA_FORCE_INLINE const _tab_t &_tab() {
		RUA_SASSERT((!std::is_same<T, void>::value));

		static const _tab_t inf{size_of<T>::value,
								align_of<T>::value,
								std::is_trivial<T>::value,
								&_desc<T>,
								_dtor<T>::value,
								_del<T>::value,
								_copy_ctor<T>::value,
								_copy_new<T>::value,
								_move_ctor<T>::value,
								_move_new<T>::value
#ifdef RUA_RTTI
								,
								&_std_id<T>
#endif
		};
		return inf;
	}

	const _tab_t *_inf;

	type_info(const _tab_t &inf) : _inf(&inf) {}

	template <typename T>
	friend RUA_FORCE_INLINE constexpr type_info type_id();
};

template <typename T>
RUA_FORCE_INLINE constexpr type_info type_id() {
	return type_info(type_info::_tab<T>());
}

template <>
RUA_FORCE_INLINE constexpr type_info type_id<void>() {
	return type_info();
}

#ifdef RUA_RTTI

RUA_FORCE_INLINE bool operator==(const type_info &a, std::type_index b) {
	return b == a.std_id();
}

RUA_FORCE_INLINE bool operator!=(const type_info &a, std::type_index b) {
	return b != a.std_id();
}

RUA_FORCE_INLINE bool operator==(std::type_index a, const type_info &b) {
	return a == b.std_id();
}

RUA_FORCE_INLINE bool operator!=(std::type_index a, const type_info &b) {
	return a != b.std_id();
}

#endif

class enable_type_info {
public:
	RUA_FORCE_INLINE type_info type() const {
		return _type;
	};

	template <typename T>
	RUA_FORCE_INLINE bool type_is() const {
		return _type == type_id<T>();
	}

protected:
	constexpr enable_type_info() : _type() {}

	constexpr enable_type_info(type_info ti) : _type(ti) {}

	type_info _type;
};

} // namespace rua

#include <functional>

namespace std {

template <>
class hash<rua::type_info> {
public:
	size_t operator()(const rua::type_info &ti) const {
		return ti.hash_code();
	}
};

} // namespace std

#endif
