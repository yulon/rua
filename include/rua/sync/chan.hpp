#ifndef _RUA_SYNC_CHAN_HPP
#define _RUA_SYNC_CHAN_HPP

#include "future.hpp"
#include "promise.hpp"

#include "../lockfree_list.hpp"
#include "../optional.hpp"
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
	constexpr chan() : _vals(), _recv_wtrs() {}

	chan(const chan &) = delete;

	chan &operator=(const chan &) = delete;

	bool send(T val) {
		optional<promise<T> *> recv_wtr;
		if (_vals.emplace_front_if(
				[this, &recv_wtr]() -> bool {
					recv_wtr = _recv_wtrs.pop_back();
					return !recv_wtr;
				},
				std::move(val))) {
			return false;
		}
		assert(recv_wtr);
		assert(*recv_wtr);
		(*recv_wtr)->fulfill(std::move(val), [this](expected<T> val) mutable {
			if (!val) {
				return;
			}
			send(std::move(val).value());
		});
		return true;
	}

	optional<T> try_recv() {
#ifdef NDEBUG
		return _vals.pop_back();
#else
		return _vals.pop_back_if_non_empty_and([this]() -> bool {
			assert(!_recv_wtrs);
			return !_recv_wtrs;
		});
#endif
	}

	future<T> recv() {
		auto val_opt = try_recv();
		if (val_opt) {
			return *std::move(val_opt);
		}

		auto prm = new newable_promise<T>;

		val_opt = _vals.pop_front_or(
			[this, prm]() { _recv_wtrs.emplace_front(prm); });
		if (!val_opt) {
			return future<T>(*prm);
		}

		prm->release();

		return *std::move(val_opt);
	}

private:
	lockfree_list<T> _vals;
	lockfree_list<promise<T> *> _recv_wtrs;
};

} // namespace rua

#endif
