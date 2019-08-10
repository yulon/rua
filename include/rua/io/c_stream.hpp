#ifndef _RUA_IO_C_STREAM_HPP
#define _RUA_IO_C_STREAM_HPP

#include "abstract.hpp"

#include <cstddef>
#include <cstdio>

namespace rua {

class c_stream : public read_write_closer {
public:
	using native_handle_t = FILE *;

	c_stream(native_handle_t f = nullptr, bool need_close = true) :
		_f(f),
		_nc(f ? need_close : false) {}

	c_stream(c_stream &&src) : c_stream(src._f, src._nc) {
		src.detach();
	}

	c_stream &operator=(c_stream &&src) {
		close();
		new (this) c_stream(std::move(src));
		return *this;
	}

	virtual ~c_stream() {
		close();
	}

	native_handle_t &native_handle() {
		return _f;
	}

	native_handle_t native_handle() const {
		return _f;
	}

	explicit operator bool() const {
		return _f;
	}

	virtual size_t read(bin_ref p) {
		return static_cast<size_t>(fread(p.base(), 1, p.size(), _f));
	}

	virtual size_t write(bin_view p) {
		return static_cast<size_t>(fwrite(p.base(), 1, p.size(), _f));
	}

	virtual void close() {
		if (!_f) {
			return;
		}
		if (_nc) {
			fclose(_f);
			_nc = false;
		}
		_f = nullptr;
	}

	void detach() {
		if (!_f) {
			return;
		}
		if (_nc) {
			_nc = false;
		}
		_f = nullptr;
	}

private:
	native_handle_t _f;
	bool _nc;
};

} // namespace rua

#endif
