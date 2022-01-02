#ifndef _RUA_SCHED_DOZER_ABSTRACT_HPP
#define _RUA_SCHED_DOZER_ABSTRACT_HPP

#include "../../interface_ptr.hpp"
#include "../../time.hpp"

#include <atomic>
#include <memory>

namespace rua {

class waker_base {
public:
	waker_base() = default;
	virtual ~waker_base() = default;

	waker_base(const waker_base &) = delete;
	waker_base(waker_base &&) = delete;
	waker_base &operator=(const waker_base &) = delete;
	waker_base &operator=(waker_base &&) = delete;

	virtual void wake() {}
};

using waker = interface_ptr<waker_base>;

class secondary_waker : public waker_base {
public:
	constexpr secondary_waker() : _pri(nullptr), _state(false) {}

	explicit secondary_waker(waker primary_waker) :
		_pri(primary_waker), _state(false) {}

	virtual ~secondary_waker() = default;

	bool state() const {
		return _state.load();
	}

	virtual void wake() {
		_state.store(true);
		if (_pri) {
			_pri->wake();
		}
	}

	void reset() {
		_state.store(false);
	}

	waker primary() const {
		return _pri;
	}

private:
	waker _pri;
	std::atomic<bool> _state;
};

class dozer_base {
public:
	virtual ~dozer_base() = default;

	dozer_base(const dozer_base &) = delete;
	dozer_base(dozer_base &&) = delete;
	dozer_base &operator=(const dozer_base &) = delete;
	dozer_base &operator=(dozer_base &&) = delete;

	virtual void yield() {
		sleep(0);
	}

	virtual void sleep(duration timeout) {
		auto start = tick();
		auto rem = timeout;
		while (doze(rem)) {
			auto now = tick();
			auto slept = now - start;
			if (slept >= timeout) {
				return;
			}
			rem = timeout - slept;
		}
	}

	virtual bool doze(duration timeout = duration_max()) {
		if (!timeout) {
			yield();
			return true;
		}
		auto t = tick();
		do {
			yield();
		} while (tick() - t < timeout);
		return true;
	}

	virtual waker get_waker() {
		static waker_base wkr;
		return wkr;
	}

	virtual bool is_own_stack() const {
		return true;
	}

protected:
	constexpr dozer_base() = default;
};

using dozer = interface_ptr<dozer_base>;

} // namespace rua

#endif
