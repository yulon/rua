#ifndef _rua_sync_lock_guard_hpp
#define _rua_sync_lock_guard_hpp

#include "await.hpp"

namespace rua {

template <typename Lock>
class lock_guard : private enable_await_operators {
public:
	explicit lock_guard(Lock &lck) : $lck(&lck), $lcking(lck.lock()) {}

	~lock_guard() {
		unlock();
	}

	lock_guard(lock_guard &&src) :
		$lck(exchange(src.$lck, nullptr)), $lcking(std::move(src.$lcking)) {}

	RUA_OVERLOAD_ASSIGNMENT(lock_guard)

	explicit operator bool() const {
		return $lck;
	}

	bool await_ready() {
		return $lcking.await_ready();
	}

	template <typename Resume>
	bool await_suspend(Resume resume) {
		assert($lck);
		return $lcking.await_suspend(std::move(resume));
	}

	lock_guard await_resume() {
		return std::move(*this);
	}

	void unlock() {
		if (!$lck) {
			return;
		}
		if ($lcking.await_ready()) {
			$lck->unlock();
		}
		$lck = nullptr;
	}

private:
	Lock *$lck;
	future<> $lcking;
};

template <typename Lock>
inline lock_guard<Lock> make_lock_guard(Lock &lck) {
	return lock_guard<Lock>(lck);
}

} // namespace rua

#endif
