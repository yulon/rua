#ifndef _RUA_CALLABLE_HPP
#define _RUA_CALLABLE_HPP

#include "types/traits.hpp"

#include <functional>
#include <vector>

namespace rua {

template <typename Callable, typename = void>
struct func_decl {};

template <typename Ret, typename... Args>
struct func_decl<Ret (*)(Args...)> {
	using type = Ret(Args...);
};

template <typename MemFuncPtr>
struct _func_decl_from_mfptr {};

template <typename T, typename Ret, typename... Args>
struct _func_decl_from_mfptr<Ret (T::*)(Args...)> {
	using type = Ret(Args...);
};

template <typename T, typename Ret, typename... Args>
struct _func_decl_from_mfptr<Ret (T::*)(Args...) const> {
	using type = Ret(Args...);
};

template <typename T, typename Ret, typename... Args>
struct _func_decl_from_mfptr<Ret (T::*)(Args...) &> {
	using type = Ret(Args...);
};

template <typename T, typename Ret, typename... Args>
struct _func_decl_from_mfptr<Ret (T::*)(Args...) &&> {
	using type = Ret(Args...);
};

template <typename T, typename Ret, typename... Args>
struct _func_decl_from_mfptr<Ret (T::*)(Args...) const &> {
	using type = Ret(Args...);
};

template <typename Callable>
struct func_decl<Callable, void_t<decltype(&Callable::operator())>> {
	using type =
		typename _func_decl_from_mfptr<decltype(&Callable::operator())>::type;
};

template <typename Callable>
using func_decl_t = typename func_decl<Callable>::type;

template <
	typename Callable,
	typename Func = std::function<func_decl_t<decay_t<Callable>>>>
inline Func to_func(Callable &&c) {
	return Func(std::forward<Callable>(c));
}

////////////////////////////////////////////////////////////////////////////

template <typename Ret, typename... Args>
class _callchain_base : public std::vector<std::function<Ret(Args...)>> {
public:
	_callchain_base() = default;
};

template <typename Callback, typename = void>
class callchain;

template <typename Ret, typename... Args>
class callchain<
	Ret(Args...),
	enable_if_t<!std::is_convertible<Ret, bool>::value>>
	: public _callchain_base<Ret, Args...> {
public:
	callchain() = default;

	void operator()(Args &&... args) const {
		for (auto &cb : *this) {
			cb(std::forward<Args>(args)...);
		}
	}
};

template <typename Ret, typename... Args>
class callchain<
	Ret(Args...),
	enable_if_t<std::is_convertible<Ret, bool>::value>>
	: public _callchain_base<Ret, Args...> {
public:
	callchain() = default;

	Ret operator()(Args &&... args) const {
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
