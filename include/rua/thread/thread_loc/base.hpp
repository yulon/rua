#ifndef _RUA_THREAD_THREAD_LOC_BASE_HPP
#define _RUA_THREAD_THREAD_LOC_BASE_HPP

#include "../this_thread_id.hpp"

#include "../../any.hpp"
#include "../../any_word.hpp"
#include "../../macros.hpp"
#include "../../sync/lf_forward_list.hpp"

#include <atomic>
#include <cassert>
#include <functional>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace rua {

class _thread_loc_indexer {
public:
	constexpr _thread_loc_indexer() : _ix_c(0), _idle_ixs() {}

	size_t alloc() {
		auto ix_opt = _idle_ixs.pop_front();
		if (ix_opt) {
			return ix_opt.value();
		}
		return _ix_c++;
	}

	void dealloc(size_t ix) {
		_idle_ixs.emplace_front(ix);
	}

private:
	std::atomic<size_t> _ix_c;
	lf_forward_list<size_t> _idle_ixs;
};

class spare_thread_loc_word {
public:
	spare_thread_loc_word(void (*dtor)(any_word)) :
		_ix(_ctx().ixer.alloc()),
		_dtor(dtor) {}

	~spare_thread_loc_word() {
		if (is_storable()) {
			return;
		}
		_ctx().ixer.dealloc(_ix);
		_ix = static_cast<size_t>(-1);
		return;
	}

	spare_thread_loc_word(spare_thread_loc_word &&src) : _ix(src._ix) {
		if (src.is_storable()) {
			src._ix = static_cast<size_t>(-1);
		}
	}

	RUA_OVERLOAD_ASSIGNMENT_R(spare_thread_loc_word)

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

		auto it = ctx.map.find(this_thread_id());
		if (it == ctx.map.end()) {
			ctx.map.emplace(
				this_thread_id(),
				std::make_pair(
					static_cast<size_t>(0), std::vector<uintptr_t>()));
			it = ctx.map.find(this_thread_id());
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

		auto it = ctx.map.find(this_thread_id());
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

		auto it = ctx.map.find(this_thread_id());
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

	using _map_t = std::
		unordered_map<thread_id_t, std::pair<size_t, std::vector<uintptr_t>>>;

	struct _ctx_t {
		_thread_loc_indexer ixer;
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
	typename ThreadLocWord,
	typename SpareThreadLocWord = spare_thread_loc_word>
class basic_thread_loc {
public:
	basic_thread_loc() : _ix(_ixer().alloc()) {}

	~basic_thread_loc() {
		_ixer().dealloc(_ix);
	}

	basic_thread_loc(basic_thread_loc &&src) : _ix(src._ix) {
		if (src.is_storable()) {
			src._ix = static_cast<size_t>(-1);
		}
	}

	RUA_OVERLOAD_ASSIGNMENT_R(basic_thread_loc)

	bool is_storable() const {
		return _ix != static_cast<size_t>(-1);
	}

	bool has_value() const {
		auto &li = _li();
		if (li.size() <= _ix) {
			return false;
		}
		return li[_ix].type_is<T>();
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
		auto &li = _li();
		if (li.size() > _ix) {
			li[_ix].reset();
		}
	}

	class loc_word_wrapper {
	public:
		loc_word_wrapper() {
			auto &word_sto = _word_sto();
			if (word_sto.is_storable()) {
				_owner = &word_sto;
				_bind<ThreadLocWord>();
				return;
			}
			auto &spare_word_sto = _spare_word_sto();
			assert(spare_word_sto.is_storable());
			_owner = &spare_word_sto;
			_bind<SpareThreadLocWord>();
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

		template <typename TLW>
		void _bind() {
			_nh = [](void *owner) -> any_word {
				return reinterpret_cast<TLW *>(owner)->native_handle();
			};
			_set = [](void *owner, any_word val) {
				reinterpret_cast<TLW *>(owner)->set(val);
			};
			_get = [](void *owner) -> any_word {
				return reinterpret_cast<TLW *>(owner)->get();
			};
			_reset = [](void *owner) {
				reinterpret_cast<TLW *>(owner)->reset();
			};
		}
	};

	static loc_word_wrapper &using_loc_word() {
		static loc_word_wrapper inst;
		return inst;
	}

private:
	size_t _ix;

	static ThreadLocWord &_word_sto() {
		static ThreadLocWord inst([](any_word val) {
			if (!val) {
				return;
			}
			val.destruct<std::vector<any>>();
		});
		return inst;
	}

	static SpareThreadLocWord &_spare_word_sto() {
		static SpareThreadLocWord inst([](any_word val) {
			if (!val) {
				return;
			}
			val.destruct<std::vector<any>>();
		});
		return inst;
	}

	static _thread_loc_indexer &_ixer() {
		static _thread_loc_indexer inst;
		return inst;
	}

	static std::vector<any> &_li() {
		auto &w_sto = using_loc_word();
		auto w = w_sto.get();
		if (!w) {
			w = std::vector<any>();
			w_sto.set(w);
		}
		return w.template as<std::vector<any>>();
	}
};

} // namespace rua

#endif
