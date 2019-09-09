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
	spare_thread_loc_word(void (*)(any_word)) : _ix(_ctx().ixer.alloc()) {}

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

	bool is_storable() const {
		return _ix != static_cast<size_t>(-1);
	}

	bool set(any_word value) {
		auto &ctx = _ctx();
		std::lock_guard<std::mutex>(ctx.mtx);

		auto it = ctx.map.find(this_thread_id());
		if (it == ctx.map.end()) {
			ctx.map.emplace(this_thread_id(), std::vector<uintptr_t>());
			it = ctx.map.find(this_thread_id());
		}
		auto &li = it->second;
		if (li.size() <= _ix) {
			li.resize(_ix + 1);
		}
		li[_ix] = value;
		return true;
	}

	any_word get() const {
		auto &ctx = _ctx();
		std::lock_guard<std::mutex>(ctx.mtx);

		auto it = ctx.map.find(this_thread_id());
		if (it == ctx.map.end()) {
			return 0;
		}
		if (it->second.size() <= _ix) {
			return 0;
		}
		return it->second[_ix];
	}

private:
	size_t _ix;

	using _map_t = std::unordered_map<thread_id_t, std::vector<uintptr_t>>;

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
	basic_thread_loc() : _ix(_ctx().ixer.alloc()) {}

	~basic_thread_loc() {
		_ctx().ixer.dealloc(_ix);
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

	template <typename... Args>
	void emplace(Args &&... args) {
		RUA_SPASSERT((std::is_constructible<T, Args...>::value));

		auto &li = _li();
		if (li.size() <= _ix) {
			li.resize(_ix + 1);
		}
		li[_ix].template emplace<T>(std::forward<Args>(args)...);
	}

	T &value() const {
		auto &li = _li();
		if (li.size() <= _ix) {
			li.resize(_ix + 1);
		}
		auto &sto = li[_ix];
		if (!sto.has_value()) {
			sto.template emplace<T>();
		}
		return sto.template as<T>();
	}

	void reset() {
		auto &li = _li();
		if (li.size() > _ix) {
			li[_ix].reset();
		}
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

	struct _ctx_t {
		void *word_sto_ptr;
		void (*set_word)(void *word_sto_ptr, any_word);
		any_word (*get_word)(void *word_sto_ptr);
		_thread_loc_indexer ixer;

		_ctx_t() : ixer() {
			auto &word_sto = _word_sto();
			if (word_sto.is_storable()) {
				word_sto_ptr = &word_sto;
				set_word = [](void *word_sto_ptr, any_word val) {
					reinterpret_cast<ThreadLocWord *>(word_sto_ptr)->set(val);
				};
				get_word = [](void *word_sto_ptr) -> any_word {
					return reinterpret_cast<ThreadLocWord *>(word_sto_ptr)
						->get();
				};
			} else {
				auto &spare_word_sto = _spare_word_sto();
				assert(spare_word_sto.is_storable());
				word_sto_ptr = &spare_word_sto;
				set_word = [](void *word_sto_ptr, any_word val) {
					reinterpret_cast<SpareThreadLocWord *>(word_sto_ptr)
						->set(val);
				};
				get_word = [](void *word_sto_ptr) -> any_word {
					return reinterpret_cast<SpareThreadLocWord *>(word_sto_ptr)
						->get();
				};
			}
		}
	};

	static _ctx_t &_ctx() {
		static _ctx_t inst;
		return inst;
	}

	static std::vector<any> &_li() {
		auto &ctx = _ctx();
		auto w = ctx.get_word(ctx.word_sto_ptr);
		if (!w) {
			w = std::vector<any>();
			ctx.set_word(ctx.word_sto_ptr, w);
		}
		return w.template as<std::vector<any>>();
	}
};

} // namespace rua

#endif
