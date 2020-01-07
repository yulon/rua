#ifndef _RUA_CALLCHAIN_HPP
#define _RUA_CALLCHAIN_HPP

#include <functional>
#include <vector>

namespace rua {

template <typename Ret, typename... Args>
class _callchain_base : public std::vector<std::function<Ret(Args...)>> {
public:
	_callchain_base() = default;
};

template <typename>
class callchain;

template <typename... Args>
class callchain<void(Args...)> : public _callchain_base<void, Args...> {
public:
	callchain() = default;

	void operator()(Args &&... args) const {
		for (auto &callee : *this) {
			callee(std::forward<Args>(args)...);
		}
	}
};

template <typename... Args>
class callchain<bool(Args...)> : public _callchain_base<bool, Args...> {
public:
	callchain() = default;

	bool operator()(Args &&... args) const {
		for (auto &callee : *this) {
			if (!callee(std::forward<Args>(args)...)) {
				return false;
			}
		}
		return true;
	}
};

template <typename Ret, typename... Args>
class callchain<Ret(Args...)> : public _callchain_base<Ret, Args...> {
public:
	callchain() = default;

	Ret operator()(Args &&... args) const {
		for (auto &callee : *this) {
			auto &&r = callee(std::forward<Args>(args)...);
			if (r) {
				return std::move(r);
			}
		}
		return Ret();
	}
};

} // namespace rua

#endif
