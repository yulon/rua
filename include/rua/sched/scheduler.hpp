#ifndef _RUA_SCHED_SCHEDULER_HPP
#define _RUA_SCHED_SCHEDULER_HPP

#include "../chrono.hpp"
#include "../ref.hpp"

#include <memory>

namespace rua {

class scheduler {
public:
	virtual ~scheduler() = default;

	virtual void yield() {
		sleep(0);
	}

	virtual void sleep(ms timeout) {
		auto t = tick();
		do {
			yield();
		} while (tick() - t < timeout);
	}

	class signaler {
	public:
		signaler() = default;
		virtual ~signaler() = default;

		virtual void signal() {}
	};

	using signaler_i = ref<signaler>;

	virtual signaler_i make_signaler() {
		static signaler wkr;
		return wkr;
	}

	virtual bool wait(signaler_i wkr, ms timeout = duration_max()) {
		if (wkr) {
			yield();
			return true;
		}
		sleep(timeout);
		return false;
	}

protected:
	constexpr scheduler() = default;
};

using scheduler_i = ref<scheduler>;

} // namespace rua

#endif
