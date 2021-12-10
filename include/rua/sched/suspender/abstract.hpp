#ifndef _RUA_SCHED_SUSPENDER_ABSTRACT_HPP
#define _RUA_SCHED_SUSPENDER_ABSTRACT_HPP

#include "../../interface_ptr.hpp"
#include "../../time.hpp"

#include <atomic>
#include <memory>

namespace rua {

class resumer {
public:
	resumer() = default;
	virtual ~resumer() = default;

	resumer(const resumer &) = delete;
	resumer(resumer &&) = delete;
	resumer &operator=(const resumer &) = delete;
	resumer &operator=(resumer &&) = delete;

	virtual void resume() {}
};

using resumer_i = interface_ptr<resumer>;

class secondary_resumer : public resumer {
public:
	constexpr secondary_resumer() : _pri(nullptr), _state(false) {}

	explicit secondary_resumer(resumer_i primary_resumer) :
		_pri(primary_resumer), _state(false) {}

	virtual ~secondary_resumer() = default;

	bool state() const {
		return _state.load();
	}

	virtual void resume() {
		_state.store(true);
		if (_pri) {
			_pri->resume();
		}
	}

	void reset() {
		_state.store(false);
	}

	resumer_i primary() const {
		return _pri;
	}

private:
	resumer_i _pri;
	std::atomic<bool> _state;
};

class suspender {
public:
	virtual ~suspender() = default;

	suspender(const suspender &) = delete;
	suspender(suspender &&) = delete;
	suspender &operator=(const suspender &) = delete;
	suspender &operator=(suspender &&) = delete;

	virtual void yield() {
		sleep(0);
	}

	virtual void sleep(duration timeout) {
		auto start = tick();
		while (suspend(timeout)) {
			auto now = tick();
			auto suspended = now - start;
			if (suspended >= timeout) {
				return;
			}
			timeout -= suspended;
		}
	}

	virtual bool suspend(duration timeout) {
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

	virtual resumer_i get_resumer() {
		static resumer rsmr;
		return rsmr;
	}

	virtual bool is_own_stack() const {
		return true;
	}

protected:
	constexpr suspender() = default;
};

using suspender_i = interface_ptr<suspender>;

} // namespace rua

#endif
