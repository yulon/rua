#ifndef _RUA_SYNC_CHAN_HPP
#define _RUA_SYNC_CHAN_HPP

#include "future.hpp"

#include "../async/result.hpp"
#include "../lockfree_list.hpp"
#include "../optional.hpp"
#include "../skater.hpp"
#include "../time/tick.hpp"
#include "../types/util.hpp"

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
		recv_opt->resolve(
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

	future<T> recv() {
		future<T> ftr;

		auto val_opt = try_recv();
		if (val_opt) {
			ftr = *std::move(val_opt);
			return ftr;
		}

		promise<T> prom;
		ftr = prom.get_future();
		val_opt = _vals.pop_front_or(
			[this, &prom]() { _recvs.emplace_front(std::move(prom)); });
		if (!val_opt) {
			return ftr;
		}
		assert(prom);
		prom.reset();
		ftr = *std::move(val_opt);
		return ftr;
	}

private:
	lockfree_list<T> _vals;
	lockfree_list<promise<T>> _recvs;
};

} // namespace rua

#endif
