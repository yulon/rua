#ifndef _rua_conc_chan_hpp
#define _rua_conc_chan_hpp

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
	constexpr chan() : $buf(), $recv_wtrs() {}

	chan(const chan &) = delete;

	chan &operator=(const chan &) = delete;

	bool send(T val) {
		optional<promise<T> *> recv_wtr;
		if ($buf.emplace_front_if_non_empty_or(
				[this, &recv_wtr]() -> bool {
					recv_wtr = $recv_wtrs.pop_back();
					return !recv_wtr;
				},
				std::move(val))) {
			return false;
		}
		$send_wtr(recv_wtr, std::move(val));
		return true;
	}

	optional<T> try_recv() {
		return
#ifdef NDEBUG
			$buf.pop_back();
#else
			$buf.pop_back_if_non_empty_and([this]() -> bool {
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

		val_opt =
			$buf.pop_back_or([this, prm]() { $recv_wtrs.emplace_front(prm); });
		if (!val_opt) {
			return future<T>(*prm);
		}

		prm->unuse();

		return *std::move(val_opt);
	}

private:
	lockfree_list<T> $buf;
	lockfree_list<promise<T> *> $recv_wtrs;

	void $send_wtr(optional<promise<T> *> &recv_wtr, T &&val) {
		assert(recv_wtr);
		assert(*recv_wtr);
		(*recv_wtr)->fulfill(std::move(val), [this](expected<T> exp) {
			if (!exp) {
				return;
			}
			$send_back(*std::move(exp));
		});
	}

	bool $send_back(T val) {
		optional<promise<T> *> recv_wtr;
		if ($buf.emplace_back_if_non_empty_or(
				[this, &recv_wtr]() -> bool {
					recv_wtr = $recv_wtrs.pop_back();
					return !recv_wtr;
				},
				std::move(val))) {
			return false;
		}
		$send_wtr(recv_wtr, std::move(val));
		return true;
	}
};

} // namespace rua

#endif
