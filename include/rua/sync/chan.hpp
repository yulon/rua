#ifndef _rua_sync_chan_hpp
#define _rua_sync_chan_hpp

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
	constexpr chan() : $vals(), $recv_wtrs() {}

	chan(const chan &) = delete;

	chan &operator=(const chan &) = delete;

	bool send(T val) {
		optional<promise<T> *> recv_wtr;
		if ($vals.emplace_front_if(
				[this, &recv_wtr]() -> bool {
					recv_wtr = $recv_wtrs.pop_back();
					return !recv_wtr;
				},
				std::move(val))) {
			return false;
		}
		assert(recv_wtr);
		assert(*recv_wtr);
		auto refunded = (*recv_wtr)->fulfill(std::move(val));
		if (refunded) {
			send(std::move(refunded).value());
		}
		return true;
	}

	optional<T> try_recv() {
#ifdef NDEBUG
		return $vals.pop_back();
#else
		return $vals.pop_back_if_non_empty_and([this]() -> bool {
			assert(!$recv_wtrs);
			return !$recv_wtrs;
		});
#endif
	}

	future<T> recv() {
		auto val_opt = try_recv();
		if (val_opt) {
			return *std::move(val_opt);
		}

		auto prm = new newable_promise<T>;

		val_opt = $vals.pop_front_or(
			[this, prm]() { $recv_wtrs.emplace_front(prm); });
		if (!val_opt) {
			return future<T>(*prm);
		}

		prm->destroy();

		return *std::move(val_opt);
	}

private:
	lockfree_list<T> $vals;
	lockfree_list<promise<T> *> $recv_wtrs;
};

} // namespace rua

#endif
