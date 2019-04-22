#ifndef _RUA_IO_WIN32_SYS_STREAM_HPP
#define _RUA_IO_WIN32_SYS_STREAM_HPP

#ifndef _WIN32
	#error rua::io::win32::sys_stream: not supported this platform!
#endif

#include "../abstract.hpp"

#include <windows.h>

#include <cassert>

namespace rua {
namespace io {
namespace win32 {

class sys_stream : public io::read_write_closer {
public:
	using native_handle_t = HANDLE;

	sys_stream(native_handle_t h = nullptr, bool need_close = true) : _h(h), _nc(h ? need_close : false) {}

	sys_stream(sys_stream &&src) : sys_stream(src._h, src._nc) {
		src.detach();
	}

	sys_stream &operator=(sys_stream &&src) {
		close();
		new (this) sys_stream(std::move(src));
		return *this;
	}

	virtual ~sys_stream() {
		close();
	}

	native_handle_t &native_handle() {
		return _h;
	}

	native_handle_t native_handle() const {
		return _h;
	}

	operator bool() const {
		return _h;
	}

	virtual size_t read(bin_ref p) {
		DWORD rsz;
		return ReadFile(_h, p.base(), p.size(), &rsz, nullptr) ? static_cast<size_t>(rsz) : static_cast<size_t>(0);
	}

	virtual size_t write(bin_view p) {
		DWORD wsz;
		return WriteFile(_h, p.base(), p.size(), &wsz, nullptr) ? static_cast<size_t>(wsz) : static_cast<size_t>(0);
	}

	virtual void close() {
		if (!_h) {
			return;
		}
		if (_nc) {
			CloseHandle(_h);
			_nc = false;
		}
		_h = nullptr;
	}

	void detach() {
		if (!_h) {
			return;
		}
		if (_nc) {
			_nc = false;
		}
		_h = nullptr;
	}

private:
	native_handle_t _h;
	bool _nc;
};

}
}
}

#endif