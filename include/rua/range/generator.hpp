#ifndef _RUA_RANGE_GENERATOR_HPP
#define _RUA_RANGE_GENERATOR_HPP

#ifdef __cpp_lib_coroutine

#include "iterator.hpp"

#include "../smart_co.hpp"
#include "../types/util.hpp"
#include "../variant.hpp"

#include <cassert>

namespace rua {

template <typename R>
class generator : private wandering_iterator {
public:
	struct promise_type {
		promise_type() = default;

		generator get_return_object() {
			return generator(*this);
		}

		std::suspend_never initial_suspend() {
			return {};
		}

		std::suspend_always final_suspend() noexcept {
			return {};
		}

		std::suspend_always yield_value(R value) {
			result.template emplace<R>(std::move(value));
			return {};
		}

		std::suspend_always yield_value(const generator<R> &sub_gen) {
			result.template emplace<const generator<R> *>(&sub_gen);
			return {};
		}

		void return_void() {
			result.reset();
		}

		void unhandled_exception() {}

		variant<R, const generator<R> *> result;
	};

	////////////////////////////////////////////////////////////////////////////

	constexpr generator() noexcept = default;

	operator bool() const noexcept {
		return _co && _co->result;
	}

	const generator &operator++() const {
		assert(*this);
		if (_co->result.template type_is<const generator<R> *>()) {
			auto &sub_gen = *_co->result.template as<const generator<R> *>();
			assert(sub_gen);
			++sub_gen;
			if (sub_gen) {
				return *this;
			}
			assert(!_co.done());
		}
		for (;;) {
			if (_co.done()) {
				return *this;
			}
			_co.resume();
			if (!_co->result.template type_is<const generator<R> *>()) {
				return *this;
			}
			auto &sub_gen = *_co->result.template as<const generator<R> *>();
			if (sub_gen) {
				return *this;
			}
			assert(!_co.done());
		}
		return *this;
	}

	R &operator*() const {
		assert(*this);
		if (_co->result.template type_is<const generator<R> *>()) {
			auto &sub_gen = *_co->result.template as<const generator<R> *>();
			assert(sub_gen);
			return *sub_gen;
		}
		return _co->result.template as<R>();
	}

	R *operator->() const {
		return &operator*();
	}

private:
	unique_co<promise_type> _co;

	generator(promise_type &p) : _co(p) {}
};

} // namespace rua

#endif

#endif
