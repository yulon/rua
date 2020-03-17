#ifndef _RUA_SCHED_SCHEDULER_ABSTRACT_HPP
#define _RUA_SCHED_SCHEDULER_ABSTRACT_HPP

#include "../../chrono.hpp"
#include "../../interface_ptr.hpp"

#include <atomic>
#include <memory>

namespace rua {

class waker {
public:
	waker() = default;
	virtual ~waker() = default;

	waker(const waker &) = delete;
	waker(waker &&) = delete;
	waker &operator=(const waker &) = delete;
	waker &operator=(waker &&) = delete;

	virtual void wake() {}
};

using waker_i = interface_ptr<waker>;

class secondary_waker : public waker {
public:
	constexpr secondary_waker() : _pri(nullptr), _state(false) {}

	explicit secondary_waker(waker_i primary_waker) :
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

	waker_i primary() const {
		return _pri;
	}

private:
	waker_i _pri;
	std::atomic<bool> _state;
};

class scheduler {
public:
	virtual ~scheduler() = default;

	scheduler(const scheduler &) = delete;
	scheduler(scheduler &&) = delete;
	scheduler &operator=(const scheduler &) = delete;
	scheduler &operator=(scheduler &&) = delete;

	virtual void yield() {
		sleep(0);
	}

	virtual bool sleep(ms timeout, bool wakeable = false) {
		if (wakeable) {
			yield();
			return true;
		}
		auto t = tick();
		do {
			yield();
		} while (tick() - t < timeout);
		return false;
	}

	virtual waker_i get_waker() {
		static waker wkr;
		return wkr;
	}

protected:
	constexpr scheduler() = default;
};

using scheduler_i = interface_ptr<scheduler>;

} // namespace rua

#endif
