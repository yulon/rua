#ifndef _RUA_SYNC_CHAN_HPP
#define _RUA_SYNC_CHAN_HPP

#include "awaitable.hpp"

#include "../lockfree_list.hpp"
#include "../optional.hpp"
#include "../skater.hpp"
#include "../time/tick.hpp"
#include "../util.hpp"

#include <cassert>
#include <cstdio>
#include <functional>
#include <memory>

namespace rua {

template <typename T>
class chan {
public:
	constexpr chan() : _vals(), _recvs() {}

	chan(const chan &) = delete;

	chan &operator=(const chan &) = delete;

	bool send(T val) {
		optional<promise<T>> recv_opt;
		if (_vals.emplace_front_if(
				[this, &recv_opt]() -> bool {
					recv_opt = _recvs.pop_back();
					return !recv_opt;
				},
				std::move(val))) {
			return false;
		}
		assert(recv_opt);
		recv_opt->set_value(
			std::move(val), [this](T val) mutable { send(std::move(val)); });
		return true;
	}

	optional<T> try_recv() {
#ifdef NDEBUG
		return _vals.pop_back();
#else
		return _vals.pop_back_if_non_empty_and([this]() -> bool {
			assert(!_recvs);
			return !_recvs;
		});
#endif
	}

	awaitable<T> recv() {
		awaitable<T> fut;

		auto val_opt = try_recv();
		if (val_opt) {
			fut = *std::move(val_opt);
			return fut;
		}

		promise<T> prm;
		fut = prm.get_future();
		val_opt = _vals.pop_front_or(
			[this, &prm]() { _recvs.emplace_front(std::move(prm)); });
		if (!val_opt) {
			return fut;
		}
		assert(prm);
		prm.reset();
		fut = *std::move(val_opt);
		return fut;
	}

private:
	lockfree_list<T> _vals;
	lockfree_list<promise<T>> _recvs;
};

} // namespace rua

#endif
