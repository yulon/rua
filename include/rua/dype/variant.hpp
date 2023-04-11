#ifndef _rua_dype_variant_hpp
#define _rua_dype_variant_hpp

#include "type_info.hpp"

#include "../util.hpp"

#include <array>
#include <cassert>

namespace rua {

template <typename... Types>
class _variant_base : public typed_base<> {
public:
	template <typename T>
	static constexpr bool has_type() {
		return index_of<T, Types...>::value != nullpos;
	}

	static constexpr bool has_type(const type_info &ti) {
		return $has_type<Types...>(ti);
	}

	////////////////////////////////////////////////////////////////////////

	constexpr _variant_base() : typed_base<>(), $sto() {}

	~_variant_base() {
		reset();
	}

	_variant_base(const _variant_base &src) : typed_base<>(src.type()) {
		if (!this->type()) {
			return;
		}

		assert(this->type().is_copyable());

		this->type().copy_to(&$sto[0], &src.$sto[0]);
	}

	_variant_base(_variant_base &&src) : typed_base<>(src.type()) {
		if (!this->type()) {
			return;
		}

		assert(this->type().is_moveable());

		this->type().move_to(&$sto[0], &src.$sto[0]);
		src.reset();
	}

	RUA_OVERLOAD_ASSIGNMENT(_variant_base)

	bool has_value() const {
		return $has_value(
			bool_constant<index_of<void, Types...>::value == nullpos>{});
	}

	explicit operator bool() const {
		return has_value();
	}

	template <typename T>
	enable_if_t<!std::is_void<T>::value && !is_null_pointer<T>::value, T &>
	as() & {
		RUA_SASSERT(has_type<T>());
		assert(type_is<T>());
		return *reinterpret_cast<T *>(&$sto[0]);
	}

	template <typename T>
	enable_if_t<!std::is_void<T>::value && !is_null_pointer<T>::value, T &&>
	as() && {
		return std::move(as<T>());
	}

	template <typename T>
	enable_if_t<
		!std::is_void<T>::value && !is_null_pointer<T>::value,
		const T &>
	as() const & {
		RUA_SASSERT(has_type<T>());
		assert(type_is<T>());
		return *reinterpret_cast<const T *>(&$sto[0]);
	}

	template <typename T>
	RUA_CONSTEXPR_14 enable_if_t<std::is_void<T>::value> as() const & {
		RUA_SASSERT(has_type<T>());
	}

	template <typename T>
	constexpr enable_if_t<is_null_pointer<T>::value, std::nullptr_t>
	as() const & {
		RUA_SASSERT(has_type<T>());
		return nullptr;
	}

	template <typename... Visitors>
	bool visit(Visitors &&...viss) & {
		return has_value() &&
			   $visitors_visit_types(std::forward<Visitors>(viss)...);
	}

	template <typename... Visitors>
	bool visit(Visitors &&...viss) && {
		return has_value() && std::move(*this).$visitors_visit_types(
								  std::forward<Visitors>(viss)...);
	}

	template <typename... Visitors>
	bool visit(Visitors &&...viss) const & {
		return has_value() &&
			   $visitors_visit_types(std::forward<Visitors>(viss)...);
	}

	template <typename T, typename Emplaced = convertible_t<T &&, Types...>>
	Emplaced &emplace(T &&val) & {
		reset();
		return $emplace<Emplaced>(std::forward<T>(val));
	}

	template <typename T, typename Emplaced = convertible_t<T &&, Types...>>
	Emplaced &&emplace(T &&val) && {
		reset();
		return std::move($emplace<Emplaced>(std::forward<T>(val)));
	}

	template <
		typename U,
		typename Emplaced = convertible_t<std::initializer_list<U>, Types...>>
	Emplaced &emplace(std::initializer_list<U> il) & {
		reset();
		return $emplace<Emplaced>(il);
	}

	template <
		typename U,
		typename Emplaced = convertible_t<std::initializer_list<U>, Types...>>
	Emplaced &&emplace(std::initializer_list<U> il) && {
		reset();
		return std::move($emplace<Emplaced>(il));
	}

	template <typename T, typename... Args>
	T &emplace(Args &&...args) & {
		reset();
		return $emplace<T>(std::forward<Args>(args)...);
	}

	template <typename T, typename... Args>
	T &&emplace(Args &&...args) && {
		return std::move(emplace<T>(std::forward<Args>(args)...));
	}

	template <typename T, typename U, typename... Args>
	T &emplace(std::initializer_list<U> il, Args &&...args) & {
		reset();
		return $emplace<T>(il, std::forward<Args>(args)...);
	}

	template <typename T, typename U, typename... Args>
	T &&emplace(std::initializer_list<U> il, Args &&...args) && {
		return std::move(emplace<T>(il, std::forward<Args>(args)...));
	}

	void reset() {
		if (!this->type()) {
			return;
		}
		this->type().destruct(reinterpret_cast<void *>(&$sto[0]));
		this->$type_reset();
	}

	uchar *data() {
		return &$sto[0];
	}

	const uchar *data() const {
		return &$sto[0];
	}

protected:
	alignas(
		max_align_of<Types...>::value) uchar $sto[max_size_of<Types...>::value];

	template <typename Last>
	static constexpr bool $has_type(const type_info &ti) {
		return ti == type_id<Last>();
	}

	template <typename First, typename Second, typename... Others>
	static constexpr bool $has_type(const type_info &ti) {
		return ti == type_id<First>() || $has_type<Second, Others...>(ti);
	}

	template <typename T, typename... Args>
	T &$emplace(Args &&...args) {
		RUA_SASSERT(has_type<decay_t<T>>());

		this->$type_reset<T>();
		return construct(
			*reinterpret_cast<T *>(&$sto[0]), std::forward<Args>(args)...);
	}

	template <typename OtherVariantRef, typename First>
	enable_if_t<
		!std::is_void<First>::value && or_convertible<First, Types...>::value,
		bool>
	$conv_from_other_variant(OtherVariantRef ov) {
		if (ov.type() != type_id<First>()) {
			return false;
		}
		$emplace<convertible_t<First, Types...>>(
			static_cast<OtherVariantRef>(ov).template as<First>());
		return true;
	}

	template <typename OtherVariantRef, typename First>
	enable_if_t<
		std::is_void<First>::value && or_convertible<First, Types...>::value,
		bool>
	$conv_from_other_variant(OtherVariantRef ov) {
		return ov.type() == type_id<First>();
	}

	template <typename OtherVariantRef, typename First>
	enable_if_t<!or_convertible<First, Types...>::value, bool>
	$conv_from_other_variant(OtherVariantRef) {
		return false;
	}

	template <
		typename OtherVariantRef,
		typename First,
		typename Second,
		typename... Others>
	enable_if_t<
		!std::is_void<First>::value && or_convertible<First, Types...>::value,
		bool>
	$conv_from_other_variant(OtherVariantRef ov) {
		if (ov.type() != type_id<First>()) {
			return $conv_from_other_variant<OtherVariantRef, Second, Others...>(
				static_cast<OtherVariantRef>(ov));
		}
		$emplace<convertible_t<First, Types...>>(
			static_cast<OtherVariantRef>(ov).template as<First>());
		return true;
	}

	template <
		typename OtherVariantRef,
		typename First,
		typename Second,
		typename... Others>
	enable_if_t<
		std::is_void<First>::value && or_convertible<First, Types...>::value,
		bool>
	$conv_from_other_variant(OtherVariantRef ov) {
		if (ov.type() != type_id<First>()) {
			return $conv_from_other_variant<OtherVariantRef, Second, Others...>(
				static_cast<OtherVariantRef>(ov));
		}
		return true;
	}

	template <
		typename OtherVariantRef,
		typename First,
		typename Second,
		typename... Others>
	enable_if_t<!or_convertible<First, Types...>::value, bool>
	$conv_from_other_variant(OtherVariantRef ov) {
		return $conv_from_other_variant<OtherVariantRef, Second, Others...>(
			static_cast<OtherVariantRef>(ov));
	}

	template <typename T, typename Visitor>
	enable_if_t<
		std::is_void<T>::value
			? !is_invocable<Visitor &&>::value
			: !is_invocable<Visitor &&, add_lvalue_reference_t<T>>::value,
		bool>
	$visit_as(Visitor &&) & {
		return false;
	}

	template <typename T, typename Visitor>
	enable_if_t<
		std::is_void<T>::value
			? !is_invocable<Visitor &&>::value
			: !is_invocable<Visitor &&, add_rvalue_reference_t<T>>::value,
		bool>
	$visit_as(Visitor &&) && {
		return false;
	}

	template <typename T, typename Visitor>
	enable_if_t<
		std::is_void<T>::value
			? !is_invocable<Visitor &&>::value
			: !is_invocable<Visitor &&, add_lvalue_reference_t<const T>>::value,
		bool> constexpr $visit_as(Visitor &&) const & {
		return false;
	}

	template <typename T, typename Visitor>
	enable_if_t<std::is_void<T>::value && is_invocable<Visitor &&>::value, bool>
	$visit_as(Visitor &&vis) const & {
		if (!type_is<T>()) {
			return false;
		};
		rua::invoke(std::forward<Visitor>(vis));
		return true;
	}

	template <typename T, typename Visitor>
	enable_if_t<
		!std::is_void<T>::value &&
			is_invocable<Visitor &&, add_lvalue_reference_t<T>>::value,
		bool>
	$visit_as(Visitor &&vis) & {
		if (!type_is<T>()) {
			return false;
		};
		rua::invoke(std::forward<Visitor>(vis), as<T>());
		return true;
	}

	template <typename T, typename Visitor>
	enable_if_t<
		!std::is_void<T>::value &&
			is_invocable<Visitor &&, add_rvalue_reference_t<T>>::value,
		bool>
	$visit_as(Visitor &&vis) && {
		if (!type_is<T>()) {
			return false;
		};
		rua::invoke(std::forward<Visitor>(vis), std::move(as<T>()));
		return true;
	}

	template <typename T, typename Visitor>
	enable_if_t<
		!std::is_void<T>::value &&
			is_invocable<Visitor &&, add_lvalue_reference_t<const T>>::value,
		bool>
	$visit_as(Visitor &&vis) const & {
		if (!type_is<T>()) {
			return false;
		};
		rua::invoke(std::forward<Visitor>(vis), as<T>());
		return true;
	}

	template <typename Visitor>
	constexpr bool $visit_types(Visitor &&) const & {
		return false;
	}

	template <typename Visitor, typename First, typename... Others>
	bool $visit_types(Visitor &&vis) & {
		return $visit_as<First>(std::forward<Visitor>(vis)) ||
			   $visit_types<Visitor, Others...>(std::forward<Visitor>(vis));
	}

	template <typename Visitor, typename First, typename... Others>
	bool $visit_types(Visitor &&vis) && {
		return std::move(*this).template $visit_as<First>(
				   std::forward<Visitor>(vis)) ||
			   std::move(*this).template $visit_types<Visitor, Others...>(
				   std::forward<Visitor>(vis));
	}

	template <typename Visitor, typename First, typename... Others>
	bool $visit_types(Visitor &&vis) const & {
		return $visit_as<First>(std::forward<Visitor>(vis)) ||
			   $visit_types<Visitor, Others...>(std::forward<Visitor>(vis));
	}

	constexpr bool $visitors_visit_types() const & {
		return false;
	}

	template <typename FirstVisitor, typename... OtherVisitors>
	bool $visitors_visit_types(
		FirstVisitor &&first_viss, OtherVisitors &&...other_viss) & {
		return $visit_types<FirstVisitor, Types...>(
				   std::forward<FirstVisitor>(first_viss)) ||
			   $visitors_visit_types(
				   std::forward<OtherVisitors>(other_viss)...);
	}

	template <typename FirstVisitor, typename... OtherVisitors>
	bool $visitors_visit_types(
		FirstVisitor &&first_viss, OtherVisitors &&...other_viss) && {
		return std::move(*this).template $visit_types<FirstVisitor, Types...>(
				   std::forward<FirstVisitor>(first_viss)) ||
			   std::move(*this).$visitors_visit_types(
				   std::forward<OtherVisitors>(other_viss)...);
	}

	template <typename FirstVisitor, typename... OtherVisitors>
	bool $visitors_visit_types(
		FirstVisitor &&first_viss, OtherVisitors &&...other_viss) const & {
		return $visit_types<FirstVisitor, Types...>(
				   std::forward<FirstVisitor>(first_viss)) ||
			   $visitors_visit_types(
				   std::forward<OtherVisitors>(other_viss)...);
	}

	bool $has_value(std::true_type &&) const {
		return this->type();
	}

	constexpr bool $has_value(std::false_type &&) const {
		return true;
	}
};

template <typename... Types>
class variant : public _variant_base<Types...>,
				private enable_copy_move_like<Types...> {
public:
	constexpr variant() : _variant_base<Types...>() {}

	template <typename T, typename Emplaced = convertible_t<T &&, Types...>>
	variant(T &&val) {
		this->template $emplace<Emplaced>(std::forward<T>(val));
	}

	template <
		typename U,
		typename Emplaced = convertible_t<std::initializer_list<U>, Types...>>
	variant(std::initializer_list<U> il) {
		this->template $emplace<Emplaced>(il);
	}

	template <typename T, typename... Args>
	explicit variant(in_place_type_t<T>, Args &&...args) {
		this->template $emplace<T>(std::forward<Args>(args)...);
	}

	template <typename T, typename U, typename... Args>
	explicit variant(
		in_place_type_t<T>, std::initializer_list<U> il, Args &&...args) {
		this->template $emplace<T>(il, std::forward<Args>(args)...);
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

		if (this->has_type(src.type())) {
			this->$type_reset(src.type());
			this->type().copy_to(this->data(), src.data());
			return;
		}

#ifdef NDEBUG
		no_effect(this->template $conv_from_other_variant<
				  const variant<Others...> &,
				  Others...>(src));
#else
		assert((this->template $conv_from_other_variant<
				const variant<Others...> &,
				Others...>(src)));
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

		if (this->has_type(src.type())) {
			this->$type_reset(src.type());
			this->type().move_to(this->data(), src.data());
			src.reset();
			return;
		}

		if (this->template $conv_from_other_variant<
				variant<Others...> &&,
				Others...>(std::move(src))) {
			src.reset();
		}
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

template <typename T>
struct is_variant : std::false_type {};

template <typename... Types>
struct is_variant<variant<Types...>> : std::true_type {};

} // namespace rua

#endif
