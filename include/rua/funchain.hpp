#ifndef _RUA_FUNCHAIN_HPP
#define _RUA_FUNCHAIN_HPP

#include "types/traits.hpp"

#include <functional>
#include <vector>

namespace rua {

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
