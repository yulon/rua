#ifndef _RUA_CALLABLE_HPP
#define _RUA_CALLABLE_HPP

#include "types/traits.hpp"

#include <functional>
#include <vector>

namespace rua {

template <typename Callable>
inline std::function<callable_prototype_t<decay_t<Callable>>>
wrap_callable(Callable &&c) {
	return std::forward<Callable>(c);
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
