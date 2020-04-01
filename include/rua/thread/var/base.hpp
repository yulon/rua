#ifndef _RUA_THREAD_VAR_BASE_HPP
#define _RUA_THREAD_VAR_BASE_HPP

#include "../basic.hpp"

#include "../../any.hpp"
#include "../../any_word.hpp"
#include "../../macros.hpp"
#include "../../sync/lockfree_list.hpp"
#include "../../types/util.hpp"

#include <atomic>
#include <cassert>
#include <functional>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace rua {

class _thread_var_indexer {
public:
	constexpr _thread_var_indexer() : _ix_c(0), _idle_ixs() {}

	size_t alloc() {
		auto idle_ix_opt = _idle_ixs.pop();
		if (idle_ix_opt) {
			return idle_ix_opt.value();
		}
		return _ix_c++;
	}

	void dealloc(size_t ix) {
		_idle_ixs.emplace(ix);
	}

private:
	std::atomic<size_t> _ix_c;
	lockfree_list<size_t> _idle_ixs;
};

class spare_thread_word_var {
public:
	spare_thread_word_var(void (*dtor)(any_word)) :
		_ix(_ctx().ixer.alloc()), _dtor(dtor) {}

	~spare_thread_word_var() {
		if (is_storable()) {
			return;
		}
		_ctx().ixer.dealloc(_ix);
		_ix = static_cast<size_t>(-1);
		return;
	}

	spare_thread_word_var(spare_thread_word_var &&src) : _ix(src._ix) {
		if (src.is_storable()) {
			src._ix = static_cast<size_t>(-1);
		}
	}

	RUA_OVERLOAD_ASSIGNMENT_R(spare_thread_word_var)

	using native_handle_t = size_t;

	native_handle_t native_handle() const {
		return _ix;
	}

	bool is_storable() const {
		return _ix != static_cast<size_t>(-1);
	}

	void set(any_word value) {
		auto &ctx = _ctx();
		std::lock_guard<std::mutex>(ctx.mtx);

		auto it = ctx.map.find(this_tid());
		if (it == ctx.map.end()) {
			ctx.map.emplace(
				this_tid(),
				std::make_pair(
					static_cast<size_t>(0), std::vector<uintptr_t>()));
			it = ctx.map.find(this_tid());
		}
		auto &li = it->second.second;
		if (li.size() <= _ix) {
			if (!value) {
				return;
			}
			++it->second.first;
			li.resize(_ix + 1);
		} else if (!li[_ix]) {
			if (!value) {
				return;
			}
			++it->second.first;
		} else if (li[_ix] && !value) {
			--it->second.first;
		}
		li[_ix] = value;
	}

	any_word get() const {
		auto &ctx = _ctx();
		std::lock_guard<std::mutex>(ctx.mtx);

		auto it = ctx.map.find(this_tid());
		if (it == ctx.map.end()) {
			return 0;
		}
		auto &li = it->second.second;
		if (li.size() <= _ix) {
			return 0;
		}
		return li[_ix];
	}

	void reset() {
		auto &ctx = _ctx();
		std::lock_guard<std::mutex>(ctx.mtx);

		auto it = ctx.map.find(this_tid());
		if (it == ctx.map.end()) {
			return;
		}
		auto &li = it->second.second;
		if (li.size() <= _ix) {
			return;
		}
		if (!li[_ix]) {
			return;
		}
		_dtor(li[_ix]);
		if (--it->second.first) {
			ctx.map.erase(it);
		}
	}

private:
	size_t _ix;
	void (*_dtor)(any_word);

	using _map_t =
		std::unordered_map<tid_t, std::pair<size_t, std::vector<uintptr_t>>>;

	struct _ctx_t {
		_thread_var_indexer ixer;
		std::mutex mtx;
		_map_t map;
	};

	static _ctx_t &_ctx() {
		static _ctx_t inst;
		return inst;
	}
};

template <
	typename T,
	typename ThreadWordVar,
	typename SpareThreadWordVar = spare_thread_word_var>
class basic_thread_var {
public:
	basic_thread_var() : _ix(_ixer().alloc()) {}

	~basic_thread_var() {
		_ixer().dealloc(_ix);
	}

	basic_thread_var(basic_thread_var &&src) : _ix(src._ix) {
		if (src.is_storable()) {
			src._ix = static_cast<size_t>(-1);
		}
	}

	RUA_OVERLOAD_ASSIGNMENT_R(basic_thread_var)

	bool is_storable() const {
		return _ix != static_cast<size_t>(-1);
	}

	bool has_value() const {
		auto &wv = using_word_var();
		auto w = wv.get();
		if (!w) {
			return false;
		}
		auto &li = w.template as<std::vector<any>>();
		if (li.size() <= _ix) {
			return false;
		}
		return li[_ix].template type_is<T>();
	}

	template <typename... Args>
	T &emplace(Args &&... args) {
		RUA_SPASSERT((std::is_constructible<T, Args...>::value));

		auto &li = _li();
		if (li.size() <= _ix) {
			li.resize(_ix + 1);
		}
		return li[_ix].template emplace<T>(std::forward<Args>(args)...);
	}

	T &value() const {
		return _li()[_ix].template as<T>();
	}

	void reset() {
		auto &wv = using_word_var();
		auto w = wv.get();
		if (!w) {
			return;
		}
		auto &li = w.template as<std::vector<any>>();
		if (li.size() <= _ix) {
			return;
		}
		li[_ix].reset();
	}

	class word_var_wrapper {
	public:
		word_var_wrapper() {
			auto &wv = _word_var<ThreadWordVar>();
			if (wv.is_storable()) {
				_owner = &wv;
				_bind<ThreadWordVar>();
				return;
			}
			auto &spare_wv = _word_var<SpareThreadWordVar>();
			assert(spare_wv.is_storable());
			_owner = &spare_wv;
			_bind<SpareThreadWordVar>();
		}

		any_word native_handle() const {
			return _nh(_owner);
		}

		void set(any_word val) {
			return _set(_owner, val);
		}

		any_word get() const {
			return _get(_owner);
		}

		void reset() {
			_reset(_owner);
		}

	private:
		void *_owner;
		any_word (*_nh)(void *owner);
		void (*_set)(void *owner, any_word);
		any_word (*_get)(void *owner);
		void (*_reset)(void *owner);

		template <typename TWV>
		void _bind() {
			_nh = [](void *owner) -> any_word {
				return reinterpret_cast<TWV *>(owner)->native_handle();
			};
			_set = [](void *owner, any_word val) {
				reinterpret_cast<TWV *>(owner)->set(val);
			};
			_get = [](void *owner) -> any_word {
				return reinterpret_cast<TWV *>(owner)->get();
			};
			_reset = [](void *owner) {
				reinterpret_cast<TWV *>(owner)->reset();
			};
		}
	};

	static word_var_wrapper &using_word_var() {
		static word_var_wrapper inst;
		return inst;
	}

private:
	size_t _ix;

	template <typename TWV>
	static TWV &_word_var() {
		static TWV inst([](any_word val) {
			if (!val) {
				return;
			}
			val.destruct<std::vector<any>>();
		});
		return inst;
	}

	static _thread_var_indexer &_ixer() {
		static _thread_var_indexer inst;
		return inst;
	}

	static std::vector<any> &_li() {
		auto &wv = using_word_var();
		auto w = wv.get();
		if (!w) {
			w = std::vector<any>();
			wv.set(w);
		}
		return w.template as<std::vector<any>>();
	}
};

} // namespace rua

#endif
