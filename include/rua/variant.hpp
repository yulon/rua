#ifndef _RUA_VARIANT_HPP
#define _RUA_VARIANT_HPP

#include "type_info.hpp"
#include "util.hpp"

#include <array>
#include <cassert>

namespace rua {

template <typename... Types>
class variant : public enable_type_info {
public:
	constexpr variant() : enable_type_info(), _sto() {}

	template <typename T, typename Emplaced = convertible_t<T &&, Types...>>
	variant(T &&val) {
		_emplace<Emplaced>(std::forward<T>(val));
	}

	template <
		typename U,
		typename Emplaced = convertible_t<std::initializer_list<U>, Types...>>
	variant(std::initializer_list<U> il) {
		_emplace<Emplaced>(il);
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
		typename... Others,
		typename = enable_if_t<
			!std::is_same<variant<Others...>, variant>::value &&
			disjunction<or_convertible<Others, Types...>...>::value>>
	variant(const variant<Others...> &src) {
		if (!src.type()) {
			return;
		}

		assert(src.type().is_copyable());

		if (has_type(src.type())) {
			_type = src.type();
			_type.copy_to(&_sto[0], src.data());
			return;
		}

#ifdef NDEBUG
		no_effect(_conv_ov<const variant<Others...> &, Others...>(src));
#else
		assert(RUA_ARG(_conv_ov<const variant<Others...> &, Others...>(src)));
#endif
	}

	template <
		typename... Others,
		typename = enable_if_t<
			!std::is_same<variant<Others...>, variant>::value &&
			disjunction<or_convertible<Others, Types...>...>::value>>
	variant(variant<Others...> &&src) {
		if (!src.type()) {
			return;
		}

		assert(src.type().is_moveable());

		if (has_type(src.type())) {
			_type = src.type();
			_type.move_to(&_sto[0], src.data());
			src.reset();
			return;
		}

		if (_conv_ov<variant<Others...> &&, Others...>(std::move(src))) {
			src.reset();
		}
	}

	~variant() {
		reset();
	}

	variant(const variant &src) : enable_type_info(src._type) {
		if (!_type) {
			return;
		}
		assert(_type.is_copyable());
		_type.copy_to(&_sto[0], &src._sto);
	}

	variant(variant &&src) : enable_type_info(src._type) {
		if (!_type) {
			return;
		}
		assert(_type.is_moveable());
		_type.move_to(&_sto[0], &src._sto);
	}

	RUA_OVERLOAD_ASSIGNMENT(variant)

	bool has_value() const {
		return _has_value(
			bool_constant<index_of<void, Types...>::value == nullpos>{});
	}

	explicit operator bool() const {
		return has_value();
	}

	template <typename T>
	enable_if_t<!std::is_void<T>::value, T &> as() & {
		assert(type_is<T>());
		return *reinterpret_cast<T *>(&_sto[0]);
	}

	template <typename T>
	enable_if_t<!std::is_void<T>::value, T &&> as() && {
		return std::move(as<T>());
	}

	template <typename T>
	enable_if_t<!std::is_void<T>::value, const T &> as() const & {
		assert(type_is<T>());
		return *reinterpret_cast<const T *>(&_sto[0]);
	}

	template <typename T>
	RUA_CONSTEXPR_14 enable_if_t<
		std::is_void<T>::value && index_of<void, Types...>::value != nullpos>
	as() const & {}

	template <typename... Visitors>
	bool visit(Visitors &&...viss) & {
		return has_value() &&
			   _visitors_visit_types(std::forward<Visitors>(viss)...);
	}

	template <typename... Visitors>
	bool visit(Visitors &&...viss) && {
		return has_value() && std::move(*this).template _visitors_visit_types(
								  std::forward<Visitors>(viss)...);
	}

	template <typename... Visitors>
	bool visit(Visitors &&...viss) const & {
		return has_value() &&
			   _visitors_visit_types(std::forward<Visitors>(viss)...);
	}

	template <typename T, typename Emplaced = convertible_t<T &&, Types...>>
	Emplaced &emplace(T &&val) & {
		reset();
		return _emplace<Emplaced>(std::forward<T>(val));
	}

	template <typename T, typename Emplaced = convertible_t<T &&, Types...>>
	Emplaced &&emplace(T &&val) && {
		reset();
		return std::move(_emplace<Emplaced>(std::forward<T>(val)));
	}

	template <
		typename U,
		typename Emplaced = convertible_t<std::initializer_list<U>, Types...>>
	Emplaced &emplace(std::initializer_list<U> il) & {
		reset();
		return _emplace<Emplaced>(il);
	}

	template <
		typename U,
		typename Emplaced = convertible_t<std::initializer_list<U>, Types...>>
	Emplaced &&emplace(std::initializer_list<U> il) && {
		reset();
		return std::move(_emplace<Emplaced>(il));
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

	bool has_type(const type_info &ti) {
		return _has_typ<Types...>(ti);
	}

	template <typename T>
	constexpr bool has_type() const {
		return index_of<T, Types...>::value != nullpos;
	}

	void reset() {
		if (!_type) {
			return;
		}
		_type.destruct(reinterpret_cast<void *>(&_sto[0]));
		_type.reset();
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
		return construct(
			*reinterpret_cast<T *>(&_sto[0]), std::forward<Args>(args)...);
	}

	template <typename Last>
	bool _has_typ(const type_info &ti) {
		return ti == type_id<Last>();
	}

	template <typename First, typename Second, typename... Others>
	bool _has_typ(const type_info &ti) {
		return ti == type_id<First>() || _has_typ<Second, Others...>(ti);
	}

	template <typename OV, typename First>
	enable_if_t<or_convertible<First, Types...>::value, bool> _conv_ov(OV ov) {
		if (ov.type() != type_id<First>()) {
			return false;
		}
		_emplace<convertible_t<First, Types...>>(
			std::forward<OV>(ov).template as<First>());
		return true;
	}

	template <typename OV, typename First>
	enable_if_t<!or_convertible<First, Types...>::value, bool> _conv_ov(OV) {
		return false;
	}

	template <typename OV, typename First, typename Second, typename... Others>
	enable_if_t<or_convertible<First, Types...>::value, bool> _conv_ov(OV ov) {
		if (ov.type() != type_id<First>()) {
			return _conv_ov<OV, Second, Others...>(std::forward<OV>(ov));
		}
		_emplace<convertible_t<First, Types...>>(
			std::forward<OV>(ov).template as<First>());
		return true;
	}

	template <typename OV, typename First, typename Second, typename... Others>
	enable_if_t<!or_convertible<First, Types...>::value, bool> _conv_ov(OV ov) {
		return _conv_ov<OV, Second, Others...>(std::forward<OV>(ov));
	}

	template <typename T, typename Visitor>
	enable_if_t<
		!is_invocable<Visitor &&, T &>::value,
		bool> constexpr _visit_as(Visitor &&) const & {
		return false;
	}

	template <typename T, typename Visitor>
	enable_if_t<is_invocable<Visitor &&, T &>::value, bool>
	_visit_as(Visitor &&vis) & {
		if (!type_is<T>()) {
			return false;
		};
		std::forward<Visitor>(vis)(as<T>());
		return true;
	}

	template <typename T, typename Visitor>
	enable_if_t<is_invocable<Visitor &&, T &>::value, bool>
	_visit_as(Visitor &&vis) && {
		if (!type_is<T>()) {
			return false;
		};
		std::forward<Visitor>(vis)(std::move(as<T>()));
		return true;
	}

	template <typename T, typename Visitor>
	enable_if_t<is_invocable<Visitor &&, T &>::value, bool>
	_visit_as(Visitor &&vis) const & {
		if (!type_is<T>()) {
			return false;
		};
		std::forward<Visitor>(vis)(as<T>());
		return true;
	}

	template <typename Visitor>
	constexpr bool _visit_types(Visitor &&) const & {
		return false;
	}

	template <typename Visitor, typename First, typename... Others>
	bool _visit_types(Visitor &&vis) & {
		return _visit_as<First>(std::forward<Visitor>(vis)) ||
			   _visit_types<Visitor, Others...>(std::forward<Visitor>(vis));
	}

	template <typename Visitor, typename First, typename... Others>
	bool _visit_types(Visitor &&vis) && {
		return std::move(*this).template _visit_as<First>(
				   std::forward<Visitor>(vis)) ||
			   std::move(*this).template _visit_types<Visitor, Others...>(
				   std::forward<Visitor>(vis));
	}

	template <typename Visitor, typename First, typename... Others>
	bool _visit_types(Visitor &&vis) const & {
		return _visit_as<First>(std::forward<Visitor>(vis)) ||
			   _visit_types<Visitor, Others...>(std::forward<Visitor>(vis));
	}

	constexpr bool _visitors_visit_types() const & {
		return false;
	}

	template <typename FirstVisitor, typename... OtherVisitors>
	bool _visitors_visit_types(
		FirstVisitor &&first_viss, OtherVisitors &&...other_viss) & {
		return _visit_types<FirstVisitor, Types...>(
				   std::forward<FirstVisitor>(first_viss)) ||
			   _visitors_visit_types(
				   std::forward<OtherVisitors>(other_viss)...);
	}

	template <typename FirstVisitor, typename... OtherVisitors>
	bool _visitors_visit_types(
		FirstVisitor &&first_viss, OtherVisitors &&...other_viss) && {
		return std::move(*this).template _visit_types<FirstVisitor, Types...>(
				   std::forward<FirstVisitor>(first_viss)) ||
			   std::move(*this).template _visitors_visit_types(
				   std::forward<OtherVisitors>(other_viss)...);
	}

	template <typename FirstVisitor, typename... OtherVisitors>
	bool _visitors_visit_types(
		FirstVisitor &&first_viss, OtherVisitors &&...other_viss) const & {
		return _visit_types<FirstVisitor, Types...>(
				   std::forward<FirstVisitor>(first_viss)) ||
			   _visitors_visit_types(
				   std::forward<OtherVisitors>(other_viss)...);
	}

	bool _has_value(std::true_type &&) const {
		return _type;
	}

	bool _has_value(std::false_type &&) const {
		return true;
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
