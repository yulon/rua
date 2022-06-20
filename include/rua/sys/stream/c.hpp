#ifndef _RUA_SYS_STREAM_C_HPP
#define _RUA_SYS_STREAM_C_HPP

#include "../../io/util.hpp"
#include "../../util.hpp"

#include <cstddef>
#include <cstdio>

namespace rua {

class c_stream : public stream_base {
public:
	using native_handle_t = FILE *;

	constexpr c_stream(native_handle_t f = nullptr, bool need_close = true) :
		_f(f), _nc(need_close) {}

	c_stream(c_stream &&src) : c_stream(src._f, src._nc) {
		src.detach();
	}

	RUA_OVERLOAD_ASSIGNMENT_R(c_stream)

	virtual ~c_stream() {
		close();
	}

	native_handle_t &native_handle() {
		return _f;
	}

	native_handle_t native_handle() const {
		return _f;
	}

	virtual operator bool() const {
		return _f;
	}

	virtual ptrdiff_t read(bytes_ref p) {
		return static_cast<ptrdiff_t>(fread(p.data(), 1, p.size(), _f));
	}

	virtual ptrdiff_t write(bytes_view p) {
		return static_cast<ptrdiff_t>(fwrite(p.data(), 1, p.size(), _f));
	}

	bool is_need_close() const {
		return _f && _nc;
	}

	virtual void close() {
		if (!_f) {
			return;
		}
		if (_nc) {
			fclose(_f);
		}
		_f = nullptr;
	}

	void detach() {
		_nc = false;
	}

private:
	native_handle_t _f;
	bool _nc;
};

} // namespace rua

#endif