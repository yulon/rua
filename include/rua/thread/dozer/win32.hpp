#ifndef _rua_thread_dozer_win32_hpp
#define _rua_thread_dozer_win32_hpp

#include "../../time/duration.hpp"
#include "../../util.hpp"

#include <windows.h>

#include <cassert>
#include <memory>

namespace rua { namespace win32 {

class waker {
public:
	using native_handle_t = HANDLE;

	waker() : $h(CreateEventW(nullptr, false, false, nullptr)) {}

	~waker() {
		if (!$h) {
			return;
		}
		CloseHandle($h);
		$h = nullptr;
	}

	native_handle_t native_handle() const {
		return $h;
	}

	operator bool() const {
		return $h;
	}

	void wake() {
		SetEvent($h);
	}

	void reset() {
		ResetEvent($h);
	}

private:
	HANDLE $h;
};

class dozer {
public:
	constexpr dozer() : $wkr() {}

	bool doze(duration timeout = duration_max()) {
		assert($wkr);
		assert(timeout >= 0);

		return WaitForSingleObject(
				   $wkr->native_handle(),
				   timeout.milliseconds<DWORD, INFINITE>()) != WAIT_TIMEOUT;
	}

	std::weak_ptr<waker> get_waker() {
		if ($wkr && $wkr.use_count() == 1) {
			$wkr->reset();
			return $wkr;
		}
		return assign($wkr, std::make_shared<waker>());
	}

private:
	std::shared_ptr<waker> $wkr;
};

}} // namespace rua::win32

#endif
