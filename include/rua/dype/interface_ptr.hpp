#ifndef _rua_dype_interface_ptr_hpp
#define _rua_dype_interface_ptr_hpp

#include "type_info.hpp"

#include "../util.hpp"

#include <cassert>
#include <memory>

namespace rua {

template <typename T>
class interface_ptr {
public:
	constexpr interface_ptr(std::nullptr_t = nullptr) :
		$raw(nullptr), $shared(), $ti() {}

	template <RUA_TMPL_IS_BASE_OF(T, SameBase)>
	interface_ptr(SameBase *raw_ptr, type_info ti = nullptr) {
		if (!raw_ptr) {
			$raw = nullptr;
			return;
		}
		$ti = ti ? ti : type_id<SameBase>();
		$raw = static_cast<T *>(raw_ptr);
	}

	template <RUA_TMPL_IS_BASE_OF(T, SameBase)>
	interface_ptr(SameBase &lv, type_info ti = nullptr) :
		interface_ptr(&lv, ti) {}

	template <RUA_TMPL_IS_BASE_OF(T, SameBase)>
	interface_ptr(SameBase &&rv, type_info ti = nullptr) {
		$ti = ti ? ti : type_id<SameBase>();
		$shared = std::static_pointer_cast<T>(
			std::make_shared<SameBase>(std::move(rv)));
		$raw = $shared.get();
	}

	interface_ptr(std::shared_ptr<T> base_shared_ptr, type_info ti = nullptr) {
		if (!base_shared_ptr) {
			$raw = nullptr;
			return;
		}
		$ti = ti ? ti : type_id<T>();
		$shared = std::move(base_shared_ptr);
		$raw = $shared.get();
	}

	template <RUA_TMPL_DERIVED(T, Derived)>
	interface_ptr(
		std::shared_ptr<Derived> derived_shared_ptr, type_info ti = nullptr) {
		if (!derived_shared_ptr) {
			$raw = nullptr;
			return;
		}
		$ti = ti ? ti : type_id<Derived>();
		$shared = std::static_pointer_cast<T>(std::move(derived_shared_ptr));
		$raw = $shared.get();
	}

	template <RUA_TMPL_IS_BASE_OF(T, SameBase)>
	interface_ptr(
		const std::unique_ptr<SameBase> &unique_ptr_lv,
		type_info ti = nullptr) :
		interface_ptr(unique_ptr_lv.get(), ti) {}

	template <RUA_TMPL_IS_BASE_OF(T, SameBase)>
	interface_ptr(
		std::unique_ptr<SameBase> &&unique_ptr_rv, type_info ti = nullptr) {
		if (!unique_ptr_rv) {
			$raw = nullptr;
			return;
		}
		$ti = ti ? ti : type_id<SameBase>();
		$shared.reset(static_cast<T *>(unique_ptr_rv.release()));
		$raw = $shared.get();
	}

	template <RUA_TMPL_DERIVED(T, Derived)>
	interface_ptr(const interface_ptr<Derived> &derived_interface_ptr_lv) :
		$ti(derived_interface_ptr_lv.type()) {

		if (!derived_interface_ptr_lv) {
			$raw = nullptr;
			return;
		}

		auto shared = derived_interface_ptr_lv.get_shared();
		if (shared) {
			$shared = std::static_pointer_cast<T>(std::move(shared));
			$raw = $shared.get();
			return;
		}

		auto raw = derived_interface_ptr_lv.get();
		assert(raw);
		$raw = static_cast<T *>(raw);
	}

	template <RUA_TMPL_DERIVED(T, Derived)>
	interface_ptr(interface_ptr<Derived> &&derived_interface_ptr_rv) :
		$ti(derived_interface_ptr_rv.type()) {

		if (!derived_interface_ptr_rv) {
			$raw = nullptr;
			return;
		}

		auto shared = derived_interface_ptr_rv.release_shared();
		if (shared) {
			$shared = std::static_pointer_cast<T>(std::move(shared));
			$raw = $shared.get();
			return;
		}

		auto raw = derived_interface_ptr_rv.release();
		assert(raw);
		$raw = static_cast<T *>(raw);
	}

	interface_ptr(const interface_ptr &src) :
		$raw(src.$raw), $shared(src.$shared), $ti(src.$ti) {}

	interface_ptr(interface_ptr &&src) :
		$raw(src.$raw), $shared(std::move(src.$shared)), $ti(src.$ti) {
		if (src.$raw) {
			src.$raw = nullptr;
		}
	}

	RUA_OVERLOAD_ASSIGNMENT(interface_ptr)

	~interface_ptr() {
		reset();
	}

	explicit operator bool() const {
		return is_valid($raw);
	}

	bool operator==(const interface_ptr<T> &src) const {
		return $ti == src.$ti && src.$ti.equal($raw, src.$raw);
	}

	bool operator!=(const interface_ptr<T> &src) const {
		return !(*this == src);
	}

	template <
		typename =
			enable_if_t<std::is_class<T>::value || std::is_union<T>::value>>
	T *operator->() const {
		assert($raw);
		return $raw;
	}

	T &operator*() const {
		return *$raw;
	}

	T *get() const {
		return $raw;
	}

	T *release() {
		if ($shared) {
			return nullptr;
		}
		auto raw = $raw;
		$raw = nullptr;
		return raw;
	}

	const std::shared_ptr<T> &get_shared() const {
		return $shared;
	}

	size_t use_count() const {
		return $shared.use_count();
	}

	std::shared_ptr<T> release_shared() {
		if (!$shared) {
			return nullptr;
		}
		$raw = nullptr;
		return std::move($shared);
	}

	type_info type() const {
		return $raw ? $ti : type_id<void>();
	}

	template <RUA_TMPL_IS_BASE_OF(T, SameBase)>
	bool type_is() const {
		return type() == type_id<SameBase>();
	}

	template <RUA_TMPL_DERIVED(T, Derived)>
	interface_ptr<Derived> as() const {
		assert($raw);

		if (!type_is<Derived>()) {
			return nullptr;
		}
		if (!$shared) {
			if (!$raw) {
				return nullptr;
			}
			return static_cast<Derived *>($raw);
		}
		return std::static_pointer_cast<Derived>($shared);
	}

	void reset() {
		if ($shared) {
			$shared.reset();
		}
		$raw = nullptr;
	}

private:
	T *$raw;
	std::shared_ptr<T> $shared;
	type_info $ti;
};

template <typename T>
class weak_interface {
public:
	constexpr weak_interface(std::nullptr_t = nullptr) :
		$raw(nullptr), $weak(), $ti() {}

	weak_interface(const interface_ptr<T> &r) :
		$raw(r.get_shared() ? r.get() : nullptr),
		$weak(r.get_shared()),
		$ti(r.type()) {}

	~weak_interface() {
		reset();
	}

	operator bool() const {
		return $raw || $weak.use_count();
	}

	interface_ptr<T> lock() const {
		if ($raw) {
			return interface_ptr<T>($raw, $ti);
		}
		if ($weak.use_count()) {
			return interface_ptr<T>($weak.lock(), $ti);
		}
		return nullptr;
	}

	void reset() {
		if ($weak) {
			$weak.reset();
		}
		$raw = nullptr;
	}

private:
	T *$raw;
	std::weak_ptr<T> $weak;
	type_info $ti;
};

} // namespace rua

#endif
