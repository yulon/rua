#ifndef _RUA_FIBER_HPP
#define _RUA_FIBER_HPP

#include "bytes.hpp"
#include "chrono.hpp"
#include "sched.hpp"
#include "sorted_list.hpp"
#include "sync.hpp"
#include "types/util.hpp"
#include "ucontext.hpp"

#include <array>
#include <atomic>
#include <cassert>
#include <functional>
#include <memory>
#include <queue>

namespace rua {

class fiber_executor;

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
			_ctx->end_ti.reset();
		}
		auto now = tick();
		if (dur >= time_max() - now) {
			_ctx->end_ti = time_max();
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
		time end_ti;

		ucontext_t _uc;
		int stk_ix;
		bytes stk_bak;

		bool has_yielded;
		std::shared_ptr<secondary_resumer> rsmr;
	};

	std::shared_ptr<_ctx_t> _ctx;

	fiber(std::shared_ptr<_ctx_t> ctx) : _ctx(std::move(ctx)) {}

	friend fiber_executor;
};

class fiber_executor {
public:
	fiber_executor(size_t stack_size = 0x100000) :
		_stk_sz(stack_size), _stk_ix(0), _spdr(*this) {}

	fiber execute(std::function<void()> task, duration lifetime = 0) {
		fiber fbr(std::make_shared<fiber::_ctx_t>());
		fbr._ctx->tsk = std::move(task);
		fbr._ctx->is_stoped.store(false);
		fbr.reset_lifetime(lifetime);
		_exs.emplace(fbr);
		return fbr;
	}

	fiber executing() const {
		return _cur;
	}

	operator bool() const {
		return _exs.size() || _spds.size() || _cws.size();
	}

	// Does not block the current context.
	// The current suspender will not be used.
	// Mostly used for frame tasks.
	void step() {
		auto now = tick();

		if (_spds.size()) {
			_check_spds(now);
		}
		if (_cws.size()) {
			_check_cws(now);
		}
		if (_exs.empty()) {
			return;
		}

		suspender_guard sg(_spdr);
		_switch_to_runner_uc();
	}

	// May block the current context.
	// The current suspender will be used.
	void run() {
		if (_exs.empty() && _spds.empty() && _cws.empty()) {
			return;
		}

		suspender_guard sg(_spdr);
		auto orig_spdr = sg.previous();
		_orig_rsmr = orig_spdr->get_resumer();

		auto now = tick();

		if (_spds.size()) {
			_check_spds(now);
		}
		if (_cws.size()) {
			_check_cws(now);
		}

		for (;;) {
			_switch_to_runner_uc();

			now = tick();

			if (_spds.size()) {

				if (_cws.size()) {
					time resume_ti;
					if (_cws.begin()->resume_ti < _spds.begin()->resume_ti) {
						resume_ti = _cws.begin()->resume_ti;
					} else {
						resume_ti = _spds.begin()->resume_ti;
					}
					if (resume_ti > now) {
						orig_spdr->suspend(resume_ti - now);
					} else {
						orig_spdr->yield();
					}
					now = tick();
					if (now >= resume_ti) {
						_check_spds(now);
					}
					_check_cws(now);
					continue;
				}

				auto resume_ti = _spds.begin()->resume_ti;
				if (resume_ti > now) {
					orig_spdr->sleep(resume_ti - now);
				} else {
					orig_spdr->yield();
				}
				now = tick();
				_check_spds(now);
				continue;

			} else if (_cws.size()) {

				auto resume_ti = _cws.begin()->resume_ti;
				if (resume_ti > now) {
					orig_spdr->suspend(resume_ti - now);
				} else {
					orig_spdr->yield();
				}
				now = tick();
				_check_cws(now);

			} else {
				return;
			}
		}
	}

	class suspender : public rua::suspender {
	public:
		suspender() = default;

		suspender(fiber_executor &fe) : _fe(&fe) {}

		virtual ~suspender() = default;

		template <typename SleepingList>
		void _suspend(SleepingList &sl, duration timeout) {
			time resume_ti;
			if (!timeout) {
				resume_ti.reset();
			} else if (timeout >= time_max() - tick()) {
				resume_ti = time_max();
			} else {
				resume_ti = tick() + timeout;
			}

			_fe->_cur._ctx->has_yielded = true;
			_fe->_cur._ctx->stk_ix = _fe->_stk_ix;
			sl.emplace(resume_ti, _fe->_cur);

			_fe->_prev = std::move(_fe->_cur);
			if (_fe->_exs.size()) {
				if (!_fe->_try_resume_exs_front()) {
					_fe->_swap_new_runner_uc(&_fe->_prev._ctx->_uc);
				}
			} else {
				swap_ucontext(&_fe->_prev._ctx->_uc, &_fe->_orig_uc);
			}
			_fe->_clear_prev();
		}

		virtual void sleep(duration timeout) {
			_suspend(_fe->_spds, timeout);
		}

		virtual bool suspend(duration timeout) {
			auto &rsmr = _fe->_cur._ctx->rsmr;
			if (!rsmr->state()) {
				_suspend(_fe->_cws, timeout);
			}
			return rsmr->state();
		}

		virtual resumer_i get_resumer() {
			assert(_fe->_cur._ctx);

			auto &rsmr = _fe->_cur._ctx->rsmr;
			if (rsmr) {
				rsmr.reset();
			} else {
				rsmr = std::make_shared<secondary_resumer>(_fe->_orig_rsmr);
			}
			return rsmr;
		}

		virtual bool is_own_stack() const {
			return false;
		}

		fiber_executor &get_executor() {
			return *_fe;
		}

	private:
		fiber_executor *_fe;
	};

	suspender &get_suspender() {
		return _spdr;
	}

private:
	std::queue<fiber> _exs;
	fiber _cur, _prev;

	struct _suspending_t {
		time resume_ti;
		fiber fbr;

		bool operator<(const _suspending_t &s) const {
			return resume_ti < s.resume_ti;
		}
	};
	sorted_list<_suspending_t> _spds;
	sorted_list<_suspending_t> _cws;

	void _check_spds(time now) {
		for (auto it = _spds.begin(); it != _spds.end(); it = _spds.erase(it)) {
			if (now < it->resume_ti) {
				break;
			}
			_exs.emplace(std::move(it->fbr));
		}
	}

	void _check_cws(time now) {
		for (auto it = _cws.begin(); it != _cws.end();) {
			if (it->fbr._ctx->rsmr->state() || now >= it->resume_ti) {
				_exs.emplace(std::move(it->fbr));
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
		_prev._ctx->stk_bak.resize(stk_used.size() + (rmdr ? 1024 - rmdr : 0));
		_prev._ctx->stk_bak = stk_used;

		_prev._ctx.reset();
	}

	bool _try_resume_exs_front(ucontext_t *oucp = nullptr) {
		if (!_exs.front()._ctx->stk_bak.size()) {
			return false;
		}

		if (oucp != &_orig_uc && _exs.front()._ctx->stk_ix == _stk_ix) {
			if (!oucp) {
				set_ucontext(&_orig_uc);
				return true;
			}
			swap_ucontext(oucp, &_orig_uc);
			return true;
		}

		_cur = std::move(_exs.front());
		_exs.pop();

		_stk_ix = _cur._ctx->stk_ix;
		auto &cur_stk = _cur_stk();
		cur_stk(cur_stk.size() - _cur._ctx->stk_bak.size())
			.copy_from(_cur._ctx->stk_bak);
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

		while (_exs.size()) {
			assert(_exs.front()._ctx);
			_try_resume_exs_front();

			_cur = std::move(_exs.front());
			assert(!_exs.front()._ctx);
			_exs.pop();

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
					_spds.emplace(time_zero(), std::move(_cur));
					break;
				}
				_cur._ctx->has_yielded = false;
			}
		}

		set_ucontext(&_orig_uc);
	}

	static void _runner(any_word th1s) {
		th1s.as<fiber_executor *>()->_run();
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
		while (_exs.size()) {
			if (!_try_resume_exs_front(&_orig_uc)) {
				_swap_new_runner_uc(&_orig_uc);
			}
			_clear_prev();
		}
	}

	friend suspender;
	suspender _spdr;

	resumer_i _orig_rsmr;
};

inline fiber_executor *this_fiber_executor() {
	auto spdr = this_suspender();
	if (!spdr) {
		return nullptr;
	}
	auto fs = spdr.as<fiber_executor::suspender>();
	if (!fs) {
		return nullptr;
	}
	return &fs->get_executor();
}

inline fiber this_fiber() {
	auto fe = this_fiber_executor();
	if (fe) {
		return fe->executing();
	}
	return fiber();
}

inline fiber co(std::function<void()> task, duration lifetime = 0) {
	auto fe = this_fiber_executor();
	if (fe) {
		return fe->execute(std::move(task), lifetime);
	}
	auto tmp_fe = std::make_shared<fiber_executor>();
	tmp_fe->execute(std::move(task), lifetime);
	tmp_fe->run();
	return fiber();
}

} // namespace rua

#endif
