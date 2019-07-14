#ifndef _RUA_UNI_TLS_HPP
#define _RUA_UNI_TLS_HPP

#include "../thread.hpp"
#include "../any_word.hpp"
#include "../macros.hpp"

#include <unordered_map>
#include <vector>
#include <queue>
#include <atomic>
#include <mutex>

namespace rua {
namespace uni {

class tls {
public:
	using native_handle_t = void *;

	constexpr tls(std::nullptr_t) : _ix(static_cast<size_t>(-1)) {}

	tls() : _ix(_alloc()) {}

	~tls() {
		free();
	}

	tls(const tls &) = delete;

	tls &operator=(const tls &) = delete;

	tls(tls &&src) : _ix(src._ix) {
		if (src) {
			src._ix = static_cast<size_t>(-1);
		}
	}

	RUA_OVERLOAD_ASSIGNMENT_R(tls)

	native_handle_t native_handle() const {
		return nullptr;
	}

	operator bool() const {
		return _ix != static_cast<size_t>(-1);
	}

	bool set(any_word value) {
		auto &s_ctx = _s_ctx();
		std::lock_guard<std::mutex>(s_ctx.mtx);

		auto it = s_ctx.map.find(thread::current_id());
		if (it == s_ctx.map.end()) {
			s_ctx.map.emplace(thread::current_id(), std::vector<uintptr_t>());
			it = s_ctx.map.find(thread::current_id());
		}
		auto &li = it->second;
		if (li.size() <= _ix) {
			li.resize(_ix + 1);
		}
		li[_ix] = value;
		return true;
	}

	any_word get() const {
		auto &s_ctx = _s_ctx();
		std::lock_guard<std::mutex>(s_ctx.mtx);

		auto it = s_ctx.map.find(thread::current_id());
		if (it == s_ctx.map.end()) {
			return 0;
		}
		if (it->second.size() <= _ix) {
			return 0;
		}
		return it->second[_ix];
	}

	bool alloc() {
		if (!free()) {
			return false;
		}
		_ix = _alloc();
		return true;
	}

	bool free() {
		if (!*this) {
			return true;
		}
		auto &s_ctx = _s_ctx();
		{
			std::lock_guard<std::mutex>(s_ctx.mtx);
			s_ctx.idle_ixs.emplace(_ix);
		}
		_ix = static_cast<size_t>(-1);
		return true;
	}

private:
	size_t _ix;

	size_t _alloc() {
		auto &s_ctx = _s_ctx();
		if (s_ctx.mtx.try_lock()) {
			if (s_ctx.idle_ixs.size()) {
				auto ix = s_ctx.idle_ixs.front();
				s_ctx.idle_ixs.pop();
				s_ctx.mtx.unlock();
				return ix;
			}
			s_ctx.mtx.unlock();
		}
		return ++_s_ctx().ix_c;
	}

	using _map_t = std::unordered_map<thread::id_t, std::vector<uintptr_t>>;

	struct _s_ctx_t {
		std::atomic<size_t> ix_c;
		std::mutex mtx;
		_map_t map;
		std::queue<size_t> idle_ixs;

		_s_ctx_t() : ix_c(0) {}
	};

	static _s_ctx_t &_s_ctx() {
		static _s_ctx_t inst;
		return inst;
	}
};

}
}

#endif
