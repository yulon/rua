#ifndef _RUA_SCHED_SCHEDULER_ABSTRACT_HPP
#define _RUA_SCHED_SCHEDULER_ABSTRACT_HPP

#include "../../chrono.hpp"
#include "../../ref.hpp"

#include <memory>

namespace rua {

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

		signaler(const signaler &) = delete;
		signaler(signaler &&) = delete;
		signaler &operator=(const signaler &) = delete;
		signaler &operator=(signaler &&) = delete;

		virtual void signal() {}
	};

	using signaler_i = ref<signaler>;

	virtual signaler_i get_signaler() {
		static signaler wkr;
		return wkr;
	}

	virtual bool wait(ms = duration_max()) {
		yield();
		return true;
	}

protected:
	constexpr scheduler() = default;
};

using scheduler_i = ref<scheduler>;

} // namespace rua

#endif
