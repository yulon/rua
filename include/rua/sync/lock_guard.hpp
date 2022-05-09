#ifndef _RUA_SYNC_LOCK_GUARD_HPP
#define _RUA_SYNC_LOCK_GUARD_HPP

#include "wait.hpp"

namespace rua {

template <typename Lock>
class lock_guard : private enable_wait_operator {
public:
	explicit lock_guard(Lock &lck) : _lck(&lck), _lcking(lck.lock()) {}

	~lock_guard() {
		unlock();
	}

	lock_guard(lock_guard &&src) :
		_lck(exchange(src._lck, nullptr)), _lcking(std::move(src._lcking)) {}

	RUA_OVERLOAD_ASSIGNMENT_R(lock_guard)

	explicit operator bool() const {
		return _lck;
	}

	bool await_ready() {
		return _lcking.await_ready();
	}

	template <typename Resume>
	bool await_suspend(Resume resume) {
		assert(_lck);
		return _lcking.await_suspend(std::move(resume));
	}

	lock_guard await_resume() {
		return std::move(*this);
	}

	void unlock() {
		if (!_lck) {
			return;
		}
		if (_lcking.await_ready()) {
			_lck->unlock();
		}
		_lck = nullptr;
	}

private:
	Lock *_lck;
	late<> _lcking;
};

template <typename Lock>
inline lock_guard<Lock> make_lock_guard(Lock &lck) {
	return lock_guard<Lock>(lck);
}

} // namespace rua

#endif
