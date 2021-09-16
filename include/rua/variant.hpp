#ifndef _RUA_VARIANT_HPP
#define _RUA_VARIANT_HPP

#include "types.hpp"

#include <cassert>

namespace rua {

template <typename... Types>
class variant : public enable_type_info {
public:
	constexpr variant() : enable_type_info(), _sto() {}

	template <
		typename T,
		typename =
			enable_if_t<index_of<decay_t<T>, Types...>::value != nullpos>>
	variant(T &&val) {
		_emplace<decay_t<T>>(std::forward<T>(val));
	}

	template <typename T, typename... Args>
	explicit variant(in_place_type_t<T>, Args &&...args) {
		_emplace<T>(std::forward<Args>(args)...);
	}

	template <typename T, typename U, typename... Args>
	explicit variant(
		in_place_type_t<T>, std::initializer_list<U> il, Args &&...args) {
		_emplace<T>(il, std::forward<Args>(args)...);
	}

	template <
		typename... Subset,
		typename = enable_if_t<
			!std::is_same<variant<Subset...>, variant>::value &&
			conjunction<bool_constant<
				index_of<Subset, Types...>::value != nullpos>...>::value>>
	variant(const variant<Subset...> &src) : enable_type_info(src.type()) {
		if (!_type) {
			return;
		}
		assert(_type.is_copyable());
		_type.copy_ctor(&_sto[0], &src.data());
	}

	template <
		typename... Subset,
		typename = enable_if_t<
			!std::is_same<variant<Subset...>, variant>::value &&
			conjunction<bool_constant<
				index_of<Subset, Types...>::value != nullpos>...>::value>>
	variant(variant<Subset...> &&src) : enable_type_info(src.type()) {
		if (!_type) {
			return;
		}
		assert(_type.is_moveable());
		_type.move_ctor(&_sto[0], &src.data());
	}

	~variant() {
		reset();
	}

	variant(const variant &src) : enable_type_info(src._type) {
		if (!_type) {
			return;
		}
		assert(_type.is_copyable());
		_type.copy_ctor(&_sto[0], &src._sto);
	}

	variant(variant &&src) : enable_type_info(src._type) {
		if (!_type) {
			return;
		}
		assert(_type.is_moveable());
		_type.move_ctor(&_sto[0], &src._sto);
	}

	RUA_OVERLOAD_ASSIGNMENT(variant)

	bool has_value() const {
		return _type;
	}

	operator bool() const {
		return _type;
	}

	template <typename T>
	T &as() & {
		assert(type_is<T>());
		return *reinterpret_cast<T *>(&_sto[0]);
	}

	template <typename T>
	T &&as() && {
		return std::move(as<T>());
	}

	template <typename T>
	const T &as() const & {
		assert(type_is<T>());
		return *reinterpret_cast<const T *>(&_sto[0]);
	}

	constexpr bool visit() const {
		return false;
	}

	template <typename FirstVisitor, typename... OtherVisitors>
	bool visit(FirstVisitor &&first_vis, OtherVisitors &&...other_vis) & {
		if (_as_all_and_visit<FirstVisitor, Types...>(
				std::forward<FirstVisitor>(first_vis))) {
			return true;
		}
		return visit(std::forward<OtherVisitors>(other_vis)...);
	}

	template <typename FirstVisitor, typename... OtherVisitors>
	bool visit(FirstVisitor &&first_vis, OtherVisitors &&...other_vis) && {
		if (std::move(*this).template _as_all_and_visit<FirstVisitor, Types...>(
				std::forward<FirstVisitor>(first_vis))) {
			return true;
		}
		return std::move(*this).template visit(
			std::forward<OtherVisitors>(other_vis)...);
	}

	template <typename FirstVisitor, typename... OtherVisitors>
	bool visit(FirstVisitor &&first_vis, OtherVisitors &&...other_vis) const & {
		if (_as_all_and_visit<FirstVisitor, Types...>(
				std::forward<FirstVisitor>(first_vis))) {
			return true;
		}
		return visit(std::forward<OtherVisitors>(other_vis)...);
	}

	template <typename T, typename... Args>
	T &emplace(Args &&...args) & {
		reset();
		return _emplace<T>(std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	T &&emplace(Args &&...args) && {
		return std::move(emplace<T>(std::forward<Args>(args)...));
	}

	template <typename T, typename U, typename... Args>
	T &emplace(std::initializer_list<U> il, Args &&...args) & {
		reset();
		return _emplace<T>(il, std::forward<Args>(args)...);
	}

	template <typename T, typename U, typename... Args>
	T &&emplace(std::initializer_list<U> il, Args &&...args) && {
		return std::move(emplace<T>(il, std::forward<Args>(args)...));
	}

	void reset() {
		if (!_type) {
			return;
		}
		_type.dtor(reinterpret_cast<void *>(&_sto[0]));
		_type = type_id<void>();
	}

	uchar *data() {
		return &_sto[0];
	}

	const uchar *data() const {
		return &_sto[0];
	}

private:
	alignas(
		max_align_of<Types...>::value) uchar _sto[max_size_of<Types...>::value];

	template <typename T, typename... Args>
	T &_emplace(Args &&...args) {
		RUA_SPASSERT((index_of<decay_t<T>, Types...>::value != nullpos));

		_type = type_id<T>();
		return *(new (&_sto[0]) T(std::forward<Args>(args)...));
	}

	template <typename T, typename Visitor>
	enable_if_t<
		!is_invocable<Visitor, T &>::value,
		bool> constexpr _as_and_visit(Visitor &&) const {
		return false;
	}

	template <typename T, typename Visitor>
	enable_if_t<is_invocable<Visitor, T &>::value, bool>
	_as_and_visit(Visitor &&vis) & {
		if (!type_is<T>()) {
			return false;
		};
		std::forward<Visitor>(vis)(as<T>());
		return true;
	}

	template <typename T, typename Visitor>
	enable_if_t<is_invocable<Visitor, T &>::value, bool>
	_as_and_visit(Visitor &&vis) && {
		if (!type_is<T>()) {
			return false;
		};
		std::forward<Visitor>(vis)(std::move(as<T>()));
		return true;
	}

	template <typename T, typename Visitor>
	enable_if_t<is_invocable<Visitor, T &>::value, bool>
	_as_and_visit(Visitor &&vis) const & {
		if (!type_is<T>()) {
			return false;
		};
		std::forward<Visitor>(vis)(as<T>());
		return true;
	}

	template <typename Visitor>
	constexpr bool _as_all_and_visit(Visitor &&) const {
		return false;
	}

	template <typename Visitor, typename FirstType, typename... OtherTypes>
	bool _as_all_and_visit(Visitor &&vis) & {
		if (_as_and_visit<FirstType>(std::forward<Visitor>(vis))) {
			return true;
		}
		return _as_all_and_visit<Visitor, OtherTypes...>(
			std::forward<Visitor>(vis));
	}

	template <typename Visitor, typename FirstType, typename... OtherTypes>
	bool _as_all_and_visit(Visitor &&vis) && {
		if (std::move(*this).template _as_and_visit<FirstType>(
				std::forward<Visitor>(vis))) {
			return true;
		}
		return std::move(*this)
			.template _as_all_and_visit<Visitor, OtherTypes...>(
				std::forward<Visitor>(vis));
	}

	template <typename Visitor, typename FirstType, typename... OtherTypes>
	bool _as_all_and_visit(Visitor &&vis) const & {
		if (_as_and_visit<FirstType>(std::forward<Visitor>(vis))) {
			return true;
		}
		return _as_all_and_visit<Visitor, OtherTypes...>(
			std::forward<Visitor>(vis));
	}
};

template <typename T, typename... Args>
variant<T> make_variant(Args &&...args) {
	return variant<T>(in_place_type_t<T>{}, std::forward<Args>(args)...);
}

template <typename T, typename U, typename... Args>
variant<T> make_variant(std::initializer_list<U> il, Args &&...args) {
	return variant<T>(in_place_type_t<T>{}, il, std::forward<Args>(args)...);
}

} // namespace rua

#endif
