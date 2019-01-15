#ifndef _RUA_IO_WIN32_HPP
#define _RUA_IO_WIN32_HPP

#ifndef _WIN32
	#error rua::io::win32: not supported this platform!
#endif

#include "abstract.hpp"

#include <windows.h>

#include <cassert>

namespace rua {
namespace io {

class win32_stream : public io::read_write_closer {
public:
	using native_handle_t = HANDLE;

	win32_stream(native_handle_t h = nullptr, bool need_close = true) : _h(h), _nc(h ? need_close : false) {}

	win32_stream(win32_stream &&src) : win32_stream(src._h, src._nc) {
		src.detach();
	}

	win32_stream &operator=(win32_stream &&src) {
		close();
		new (this) win32_stream(std::move(src));
		return *this;
	}

	virtual ~win32_stream() {
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

#endif
