#ifndef _rua_thread_var_base_hpp
#define _rua_thread_var_base_hpp

#include "../id.hpp"

#include "../../dype/any.hpp"
#include "../../dype/any_word.hpp"
#include "../../lockfree_list.hpp"
#include "../../util.hpp"

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
	constexpr _thread_var_indexer() : $ix_c(0), $idle_ixs() {}

	size_t alloc() {
		auto idle_ix_opt = $idle_ixs.pop_front();
		if (idle_ix_opt) {
			return idle_ix_opt.value();
		}
		return $ix_c++;
	}

	void dealloc(size_t ix) {
		$idle_ixs.emplace_front(ix);
	}

private:
	std::atomic<size_t> $ix_c;
	lockfree_list<size_t> $idle_ixs;
};

class spare_thread_word_var {
public:
	spare_thread_word_var(void (*dtor)(any_word)) :
		$ix($ctx().ixer.alloc()), $dtor(dtor) {}

	~spare_thread_word_var() {
		if (is_storable()) {
			return;
		}
		$ctx().ixer.dealloc($ix);
		$ix = static_cast<size_t>(-1);
		return;
	}

	spare_thread_word_var(spare_thread_word_var &&src) : $ix(src.$ix) {
		if (src.is_storable()) {
			src.$ix = static_cast<size_t>(-1);
		}
	}

	RUA_OVERLOAD_ASSIGNMENT(spare_thread_word_var)

	using native_handle_t = size_t;

	native_handle_t native_handle() const {
		return $ix;
	}

	bool is_storable() const {
		return $ix != static_cast<size_t>(-1);
	}

	void set(any_word value) {
		auto &ctx = $ctx();
		std::lock_guard<std::mutex> lg(ctx.mtx);

		auto it = ctx.map.find(this_tid());
		if (it == ctx.map.end()) {
			ctx.map.emplace(
				this_tid(),
				std::make_pair(
					static_cast<size_t>(0), std::vector<uintptr_t>()));
			it = ctx.map.find(this_tid());
		}
		auto &li = it->second.second;
		if (li.size() <= $ix) {
			if (!value) {
				return;
			}
			++it->second.first;
			li.resize($ix + 1);
		} else if (!li[$ix]) {
			if (!value) {
				return;
			}
			++it->second.first;
		} else if (li[$ix] && !value) {
			--it->second.first;
		}
		li[$ix] = value;
	}

	any_word get() const {
		auto &ctx = $ctx();
		std::lock_guard<std::mutex> lg(ctx.mtx);

		auto it = ctx.map.find(this_tid());
		if (it == ctx.map.end()) {
			return 0;
		}
		auto &li = it->second.second;
		if (li.size() <= $ix) {
			return 0;
		}
		return li[$ix];
	}

	void reset() {
		auto &ctx = $ctx();
		std::lock_guard<std::mutex> lg(ctx.mtx);

		auto it = ctx.map.find(this_tid());
		if (it == ctx.map.end()) {
			return;
		}
		auto &li = it->second.second;
		if (li.size() <= $ix) {
			return;
		}
		if (!li[$ix]) {
			return;
		}
		$dtor(li[$ix]);
		if (--it->second.first) {
			ctx.map.erase(it);
		}
	}

private:
	size_t $ix;
	void (*$dtor)(any_word);

	using $map_t =
		std::unordered_map<tid_t, std::pair<size_t, std::vector<uintptr_t>>>;

	struct $ctx_t {
		_thread_var_indexer ixer;
		std::mutex mtx;
		$map_t map;
	};

	static $ctx_t &$ctx() {
		static $ctx_t inst;
		return inst;
	}
};

template <
	typename T,
	typename ThreadWordVar,
	typename SpareThreadWordVar = spare_thread_word_var>
class basic_thread_var {
public:
	basic_thread_var() : $ix($ixer().alloc()) {}

	~basic_thread_var() {
		$ixer().dealloc($ix);
	}

	basic_thread_var(basic_thread_var &&src) : $ix(src.$ix) {
		if (src.is_storable()) {
			src.$ix = static_cast<size_t>(-1);
		}
	}

	RUA_OVERLOAD_ASSIGNMENT(basic_thread_var)

	bool is_storable() const {
		return $ix != static_cast<size_t>(-1);
	}

	bool has_value() const {
		auto &wv = using_word_var();
		auto w = wv.get();
		if (!w) {
			return false;
		}
		auto &li = w.template as<std::vector<any>>();
		if (li.size() <= $ix) {
			return false;
		}
		return li[$ix].template type_is<T>();
	}

	template <typename... Args>
	T &emplace(Args &&...args) {
		RUA_SASSERT(std::is_constructible<T, Args...>::value);

		auto &li = $li();
		if (li.size() <= $ix) {
			li.resize($ix + 1);
		}
		return li[$ix].template emplace<T>(std::forward<Args>(args)...);
	}

	T &value() const {
		return $li()[$ix].template as<T>();
	}

	void reset() {
		auto &wv = using_word_var();
		auto w = wv.get();
		if (!w) {
			return;
		}
		auto &li = w.template as<std::vector<any>>();
		if (li.size() <= $ix) {
			return;
		}
		li[$ix].reset();
	}

	class word_var_wrapper {
	public:
		word_var_wrapper() {
			auto &wv = $word_var<ThreadWordVar>();
			if (wv.is_storable()) {
				$owner = &wv;
				$bind<ThreadWordVar>();
				return;
			}
			auto &spare_wv = $word_var<SpareThreadWordVar>();
			assert(spare_wv.is_storable());
			$owner = &spare_wv;
			$bind<SpareThreadWordVar>();
		}

		any_word native_handle() const {
			return $nh($owner);
		}

		void set(any_word val) {
			this->$set($owner, val);
			/*
				G++ 11.2.0 may have bug:
				$set($owner, val) - no
				$set($owner, val.value()) - yes
				this->$set($owner, val) - yes
			*/
		}

		any_word get() const {
			return $get($owner);
		}

		void reset() {
			$reset($owner);
		}

	private:
		void *$owner;
		any_word (*$nh)(void *owner);
		void (*$set)(void *owner, any_word);
		any_word (*$get)(void *owner);
		void (*$reset)(void *owner);

		template <typename TWV>
		void $bind() {
			$nh = [](void *owner) -> any_word {
				return reinterpret_cast<TWV *>(owner)->native_handle();
			};
			$set = [](void *owner, any_word val) {
				reinterpret_cast<TWV *>(owner)->set(val);
			};
			$get = [](void *owner) -> any_word {
				return reinterpret_cast<TWV *>(owner)->get();
			};
			$reset = [](void *owner) {
				reinterpret_cast<TWV *>(owner)->reset();
			};
		}
	};

	static word_var_wrapper &using_word_var() {
		static word_var_wrapper inst;
		return inst;
	}

private:
	size_t $ix;

	template <typename TWV>
	static TWV &$word_var() {
		static TWV inst([](any_word val) {
			if (!val) {
				return;
			}
			val.destruct<std::vector<any>>();
		});
		return inst;
	}

	static _thread_var_indexer &$ixer() {
		static _thread_var_indexer inst;
		return inst;
	}

	static std::vector<any> &$li() {
		auto &wv = using_word_var();
		auto w = wv.get();
		if (!w) {
			w = any_word(std::vector<any>());
			wv.set(w);
		}
		return w.template as<std::vector<any>>();
	}
};

} // namespace rua

#endif
