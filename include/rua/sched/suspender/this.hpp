#ifndef _RUA_SCHED_SUSPENDER_THIS_HPP
#define _RUA_SCHED_SUSPENDER_THIS_HPP

#include "abstract.hpp"
#include "default.hpp"

#include "../../macros.hpp"
#include "../../thread/var.hpp"

#include <cassert>

namespace rua {

inline suspender_i &_this_suspender_ref() {
	static thread_var<suspender_i> sto;
	if (!sto.has_value()) {
		return sto.emplace(make_default_suspender());
	}
	assert(sto.value().get());
	return sto.value();
}

inline suspender_i this_suspender() {
	return _this_suspender_ref();
}

class suspender_guard {
public:
	suspender_guard(suspender_i spdr) {
		auto &spdr_ref = _this_suspender_ref();
		_prev = std::move(spdr_ref);
		spdr_ref = std::move(spdr);
	}

	suspender_guard(const suspender_guard &) = delete;

	suspender_guard &operator=(const suspender_guard &) = delete;

	~suspender_guard() {
		_this_suspender_ref() = std::move(_prev);
	}

	suspender_i previous() {
		return _prev;
	}

private:
	suspender_i _prev;
};

inline void yield() {
	this_suspender()->yield();
}

inline void sleep(duration timeout) {
	this_suspender()->sleep(timeout);
}

} // namespace rua

#endif
