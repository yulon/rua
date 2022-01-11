#ifndef _RUA_FIBER_HPP
#define _RUA_FIBER_HPP

#include "bytes.hpp"
#include "sched.hpp"
#include "sorted_list.hpp"
#include "sync.hpp"
#include "time.hpp"
#include "types/util.hpp"
#include "ucontext.hpp"

#include <array>
#include <atomic>
#include <cassert>
#include <functional>
#include <memory>
#include <queue>

namespace rua {

class fiber_runer;
class fiber_dozer;

class fiber {
public:
	constexpr fiber() = default;

	operator bool() const {
		return _ctx && !_ctx->is_stoped.load();
	}

	void reset_lifetime(duration dur = 0) {
		if (!_ctx) {
			return;
		}
		if (!dur) {
			_ctx->end_ti = 0;
		}
		auto now = tick();
		if (dur >= duration_max() - now) {
			_ctx->end_ti = duration_max();
			return;
		}
		_ctx->end_ti = now + dur;
	}

	void stop() {
		if (!_ctx) {
			return;
		}
		_ctx->is_stoped.store(true);
		_ctx.reset();
	}

private:
	struct _ctx_t {
		std::function<void()> tsk;

		std::atomic<bool> is_stoped;
		duration end_ti;

		ucontext_t _uc;
		int stk_ix;
		bytes stk_bak;

		bool has_yielded;
		std::shared_ptr<secondary_waker> wkr;
	};

	std::shared_ptr<_ctx_t> _ctx;

	fiber(std::shared_ptr<_ctx_t> ctx) : _ctx(std::move(ctx)) {}

	friend fiber_runer;
	friend fiber_dozer;
};

class fiber_dozer : public rua::dozer_base {
public:
	fiber_dozer() = default;

	virtual ~fiber_dozer() = default;

	template <typename DozingList>
	inline void _doze(DozingList &dl, duration timeout);

	inline virtual void sleep(duration timeout);

	inline virtual bool doze(duration timeout = duration_max());

	inline virtual waker_i get_waker();

	inline virtual bool is_unowned_data(bytes_view data) const;

	fiber_runer &get_runer() {
		return *_fr;
	}

private:
	fiber_runer *_fr;

	explicit fiber_dozer(fiber_runer &fr) : _fr(&fr) {}

	friend fiber_runer;
};

class fiber_runer {
public:
	fiber_runer(size_t stack_size = 0x100000) :
		_stk_sz(stack_size), _stk_ix(0), _dzr(*this) {}

	fiber add(std::function<void()> task, duration lifetime = 0) {
		fiber fbr(std::make_shared<fiber::_ctx_t>());
		fbr._ctx->tsk = std::move(task);
		fbr._ctx->is_stoped.store(false);
		fbr.reset_lifetime(lifetime);
		_runs.emplace(fbr);
		return fbr;
	}

	fiber running() const {
		return _cur;
	}

	operator bool() const {
		return _runs.size() || _dzs.size() || _cws.size();
	}

	// Does not block the current context.
	// The current dozer will not be used.
	// Mostly used for frame tasks.
	void step() {
		auto now = tick();

		if (_dzs.size()) {
			_check_dzs(now);
		}
		if (_cws.size()) {
			_check_cws(now);
		}
		if (_runs.empty()) {
			return;
		}

		dozer_guard sg(_dzr);
		_switch_to_runner_uc();
	}

	// May block the current context.
	// The current dozer will be used.
	void run() {
		if (_runs.empty() && _dzs.empty() && _cws.empty()) {
			return;
		}

		dozer_guard sg(_dzr);
		auto orig_dzr = sg.previous();
		_orig_wkr = orig_dzr->get_waker();

		auto now = tick();

		if (_dzs.size()) {
			_check_dzs(now);
		}
		if (_cws.size()) {
			_check_cws(now);
		}

		for (;;) {
			_switch_to_runner_uc();

			now = tick();

			if (_dzs.size()) {

				if (_cws.size()) {
					duration wake_ti;
					if (_cws.begin()->wake_ti < _dzs.begin()->wake_ti) {
						wake_ti = _cws.begin()->wake_ti;
					} else {
						wake_ti = _dzs.begin()->wake_ti;
					}
					if (wake_ti > now) {
						orig_dzr->doze(wake_ti - now);
					} else {
						orig_dzr->yield();
					}
					now = tick();
					if (now >= wake_ti) {
						_check_dzs(now);
					}
					_check_cws(now);
					continue;
				}

				auto wake_ti = _dzs.begin()->wake_ti;
				if (wake_ti > now) {
					orig_dzr->sleep(wake_ti - now);
				} else {
					orig_dzr->yield();
				}
				now = tick();
				_check_dzs(now);
				continue;

			} else if (_cws.size()) {

				auto wake_ti = _cws.begin()->wake_ti;
				if (wake_ti > now) {
					orig_dzr->doze(wake_ti - now);
				} else {
					orig_dzr->yield();
				}
				now = tick();
				_check_cws(now);

			} else {
				return;
			}
		}
	}

	fiber_dozer &get_dozer() {
		return _dzr;
	}

private:
	std::queue<fiber> _runs;
	fiber _cur, _prev;

	struct _dozing_t {
		duration wake_ti;
		fiber fbr;

		bool operator<(const _dozing_t &s) const {
			return wake_ti < s.wake_ti;
		}
	};
	sorted_list<_dozing_t> _dzs;
	sorted_list<_dozing_t> _cws;

	void _check_dzs(duration now) {
		for (auto it = _dzs.begin(); it != _dzs.end(); it = _dzs.erase(it)) {
			if (now < it->wake_ti) {
				break;
			}
			_runs.emplace(std::move(it->fbr));
		}
	}

	void _check_cws(duration now) {
		for (auto it = _cws.begin(); it != _cws.end();) {
			if (it->fbr._ctx->wkr->state() || now >= it->wake_ti) {
				_runs.emplace(std::move(it->fbr));
				it = _cws.erase(it);
				continue;
			}
			++it;
		}
	}

	ucontext_t _orig_uc;

	size_t _stk_sz;
	int _stk_ix;
	bytes _stks[2];
	ucontext_t _new_runner_ucs[2];

	bytes &_cur_stk() {
		return _stks[_stk_ix];
	}

	ucontext_t &_cur_new_runner_uc() {
		return _new_runner_ucs[_stk_ix];
	}

	void _clear_prev() {
		if (!_prev) {
			return;
		}

		assert(!_prev._ctx->stk_bak.size());

		auto &stk = _stks[_prev._ctx->stk_ix];
		auto stk_used =
			stk(_prev._ctx->_uc.sp() - reinterpret_cast<uintptr_t>(stk.data()));
		auto rmdr = stk_used.size() % 1024;
		_prev._ctx->stk_bak.reset(stk_used.size() + (rmdr ? 1024 - rmdr : 0));
		_prev._ctx->stk_bak.reset(stk_used.size());
		_prev._ctx->stk_bak.copy(stk_used);

		_prev._ctx.reset();
	}

	bool _try_wake_runs_front(ucontext_t *oucp = nullptr) {
		if (!_runs.front()._ctx->stk_bak.size()) {
			return false;
		}

		if (oucp != &_orig_uc && _runs.front()._ctx->stk_ix == _stk_ix) {
			if (!oucp) {
				set_ucontext(&_orig_uc);
				return true;
			}
			swap_ucontext(oucp, &_orig_uc);
			return true;
		}

		_cur = std::move(_runs.front());
		_runs.pop();

		_stk_ix = _cur._ctx->stk_ix;
		auto &cur_stk = _cur_stk();
		cur_stk(cur_stk.size() - _cur._ctx->stk_bak.size())
			.copy(_cur._ctx->stk_bak);
		_cur._ctx->stk_bak.resize(0);

		if (!oucp) {
			set_ucontext(&_cur._ctx->_uc);
			return true;
		}
		swap_ucontext(oucp, &_cur._ctx->_uc);
		return true;
	}

	void _run() {
		_clear_prev();

		while (_runs.size()) {
			assert(_runs.front()._ctx);
			_try_wake_runs_front();

			_cur = std::move(_runs.front());
			assert(!_runs.front()._ctx);
			_runs.pop();

			if (_cur._ctx->is_stoped.load()) {
				continue;
			}

			for (;;) {
				_cur._ctx->tsk();

				if (_cur._ctx->is_stoped.load()) {
					break;
				}

				if (_cur._ctx->end_ti <= tick()) {
					_cur._ctx->is_stoped.store(false);
					break;
				}

				if (!_cur._ctx->has_yielded) {
					_dzs.emplace(0, std::move(_cur));
					break;
				}
				_cur._ctx->has_yielded = false;
			}
		}

		set_ucontext(&_orig_uc);
	}

	static void _runner(generic_word th1s) {
		th1s.as<fiber_runer *>()->_run();
	}

	void _swap_new_runner_uc(ucontext_t *oucp) {
		if (oucp != &_orig_uc) {
			_stk_ix = !_stk_ix;
		}
		auto &cur_stk = _cur_stk();
		if (!cur_stk) {
			cur_stk.reset(_stk_sz);
			get_ucontext(&_cur_new_runner_uc());
			make_ucontext(&_cur_new_runner_uc(), &_runner, this, cur_stk);
		}
		swap_ucontext(oucp, &_cur_new_runner_uc());
	}

	void _switch_to_runner_uc() {
		while (_runs.size()) {
			if (!_try_wake_runs_front(&_orig_uc)) {
				_swap_new_runner_uc(&_orig_uc);
			}
			_clear_prev();
		}
	}

	waker_i _orig_wkr;

	friend fiber_dozer;

	fiber_dozer _dzr;
};

template <typename DozingList>
inline void fiber_dozer::_doze(DozingList &dl, duration timeout) {
	duration wake_ti;
	if (!timeout) {
		wake_ti = 0;
	} else if (timeout >= duration_max() - tick()) {
		wake_ti = duration_max();
	} else {
		wake_ti = tick() + timeout;
	}

	_fr->_cur._ctx->has_yielded = true;
	_fr->_cur._ctx->stk_ix = _fr->_stk_ix;
	dl.emplace(wake_ti, _fr->_cur);

	_fr->_prev = std::move(_fr->_cur);
	if (_fr->_runs.size()) {
		if (!_fr->_try_wake_runs_front()) {
			_fr->_swap_new_runner_uc(&_fr->_prev._ctx->_uc);
		}
	} else {
		swap_ucontext(&_fr->_prev._ctx->_uc, &_fr->_orig_uc);
	}
	_fr->_clear_prev();
}

inline void fiber_dozer::sleep(duration timeout) {
	_doze(_fr->_dzs, timeout);
}

inline bool fiber_dozer::doze(duration timeout) {
	auto &wkr = _fr->_cur._ctx->wkr;
	if (!wkr->state()) {
		_doze(_fr->_cws, timeout);
	}
	return wkr->state();
}

inline waker_i fiber_dozer::get_waker() {
	assert(_fr->_cur._ctx);

	auto &wkr = _fr->_cur._ctx->wkr;
	if (wkr) {
		wkr.reset();
	} else {
		wkr = std::make_shared<secondary_waker>(_fr->_orig_wkr);
	}
	return wkr;
}

inline bool fiber_dozer::is_unowned_data(bytes_view data) const {
	auto &cur_stk = _fr->_cur_stk();
	return data.data() >= cur_stk.data() && data.data() < cur_stk.end();
}

inline fiber_runer *this_fiber_runer() {
	auto dzr = this_dozer();
	if (!dzr) {
		return nullptr;
	}
	auto fd = dzr.as<fiber_dozer>();
	if (!fd) {
		return nullptr;
	}
	return &fd->get_runer();
}

inline fiber this_fiber() {
	auto fr = this_fiber_runer();
	if (fr) {
		return fr->running();
	}
	return fiber();
}

inline fiber co(std::function<void()> task, duration lifetime = 0) {
	auto fr = this_fiber_runer();
	if (fr) {
		return fr->add(std::move(task), lifetime);
	}
	auto tmp_fr = std::make_shared<fiber_runer>();
	tmp_fr->add(std::move(task), lifetime);
	tmp_fr->run();
	return fiber();
}

} // namespace rua

#endif
