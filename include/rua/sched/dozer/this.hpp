#ifndef _RUA_SCHED_DOZER_THIS_HPP
#define _RUA_SCHED_DOZER_THIS_HPP

#include "abstract.hpp"
#include "default.hpp"

#include "../../macros.hpp"
#include "../../thread/var.hpp"

#include <cassert>

namespace rua {

inline dozer_i &_this_dozer_ref() {
	static thread_var<dozer_i> sto;
	if (!sto.has_value()) {
		return sto.emplace(make_default_dozer());
	}
	assert(sto.value().get());
	return sto.value();
}

inline dozer_i this_dozer() {
	return _this_dozer_ref();
}

class dozer_guard {
public:
	dozer_guard(dozer_i dzr) {
		auto &dzr_ref = _this_dozer_ref();
		_prev = std::move(dzr_ref);
		dzr_ref = std::move(dzr);
	}

	dozer_guard(const dozer_guard &) = delete;

	dozer_guard &operator=(const dozer_guard &) = delete;

	~dozer_guard() {
		_this_dozer_ref() = std::move(_prev);
	}

	dozer_i previous() {
		return _prev;
	}

private:
	dozer_i _prev;
};

inline void yield() {
	this_dozer()->yield();
}

inline void sleep(duration timeout) {
	this_dozer()->sleep(timeout);
}

} // namespace rua

#endif