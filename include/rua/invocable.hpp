#ifndef _rua_invocable_hpp
#define _rua_invocable_hpp

#include "util/base.hpp"
#include "util/macros.hpp"
#include "util/type_traits.hpp"

#include <functional>
#include <vector>

namespace rua {

#ifdef __cpp_lib_invoke

template <typename F, typename... Args>
inline decltype(std::invoke(std::declval<F &&>(), std::declval<Args &&>()...))
invoke(F &&f, Args &&...args) {
	return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
}

#else

template <typename Base, typename Mbr, typename Derived, typename... Args>
inline enable_if_t<
	std::is_function<Mbr>::value &&
		std::is_base_of<Base, decay_t<Derived>>::value,
	decltype((std::declval<Derived &&>().*std::declval<Mbr Base::*>())(
		std::declval<Args &&>()...))>
invoke(Mbr Base::*mbr_ptr, Derived &&ref, Args &&...args) {
	return (ref.*mbr_ptr)(std::forward<Args>(args)...);
}

template <typename Base, typename Mbr, typename Derived>
inline enable_if_t<
	!std::is_function<Mbr>::value &&
		std::is_base_of<Base, decay_t<Derived>>::value,
	decltype(std::declval<Derived &&>().*std::declval<Mbr Base::*>())>
invoke(Mbr Base::*mbr_ptr, Derived &&ref) {
	return ref.*mbr_ptr;
}

template <typename Base, typename Mbr, typename Derived, typename... Args>
inline decltype(rua::invoke(
	std::declval<Mbr Base::*>(),
	std::declval<Derived &>(),
	std::declval<Args &&>()...))
invoke(
	Mbr Base::*mbr_ptr,
	std::reference_wrapper<Derived> ref_wrap,
	Args &&...args) {
	return rua::invoke(mbr_ptr, ref_wrap.get(), std::forward<Args>(args)...);
}

template <typename Base, typename Mbr, typename Derived, typename... Args>
inline decltype(rua::invoke(
	std::declval<Mbr Base::*>(),
	std::declval<Derived &>(),
	std::declval<Args &&>()...))
invoke(Mbr Base::*mbr_ptr, Derived *ptr, Args &&...args) {
	return rua::invoke(mbr_ptr, *ptr, std::forward<Args>(args)...);
}

template <
	typename F,
	typename... Args,
	typename = enable_if_t<!std::is_member_pointer<decay_t<F>>::value>>
inline decltype(std::declval<F &&>()(std::declval<Args &&>()...))
invoke(F &&f, Args &&...args) {
	return std::forward<F>(f)(std::forward<Args>(args)...);
}

#endif

#ifdef __cpp_lib_is_invocable

template <typename F, typename... Args>
using invoke_result = std::invoke_result<F, Args...>;

template <typename F, typename... Args>
using invoke_result_t = std::invoke_result_t<F, Args...>;

#else

template <typename F, typename... Args>
using invoke_result_t =
	decltype(rua::invoke(std::declval<F>(), std::declval<Args>()...));

template <typename F, typename... Args>
struct invoke_result {
	using type = invoke_result_t<F, Args...>;
};

#endif

#ifdef __cpp_lib_is_invocable

template <typename F, typename... Args>
using is_invocable = std::is_invocable<F, Args...>;

#else

template <typename...>
struct _is_invocable : std::false_type {};

template <typename F, typename... Args>
struct _is_invocable<void_t<invoke_result_t<F, Args...>>, F, Args...>
	: std::true_type {};

template <typename F, typename... Args>
struct is_invocable : _is_invocable<void, F, Args...> {};

#endif

#ifdef RUA_HAS_INLINE_VAR
template <typename F, typename... Args>
inline constexpr auto is_invocable_v = is_invocable<F, Args...>::value;
#endif

////////////////////////////////////////////////////////////////////////////

template <typename Ret, typename... Args>
class _funchain_base : public std::vector<std::function<Ret(Args...)>> {
public:
	_funchain_base() = default;
};

template <typename Callback, typename = void>
class funchain;

template <typename Ret, typename... Args>
class funchain<
	Ret(Args...),
	enable_if_t<!std::is_convertible<Ret, bool>::value>>
	: public _funchain_base<Ret, Args...> {
public:
	funchain() = default;

	void operator()(Args &&...args) const {
		for (auto &cb : *this) {
			cb(std::forward<Args>(args)...);
		}
	}
};

template <typename Ret, typename... Args>
class funchain<Ret(Args...), enable_if_t<std::is_convertible<Ret, bool>::value>>
	: public _funchain_base<Ret, Args...> {
public:
	funchain() = default;

	Ret operator()(Args &&...args) const {
		for (auto &cb : *this) {
			auto &&r = cb(std::forward<Args>(args)...);
			if (static_cast<bool>(r)) {
				return std::move(r);
			}
		}
		return Ret();
	}
};

} // namespace rua

#endif
