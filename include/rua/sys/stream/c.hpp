#ifndef _rua_sys_stream_c_hpp
#define _rua_sys_stream_c_hpp

#include "../../io/util.hpp"
#include "../../util.hpp"

#include <cstddef>
#include <cstdio>

namespace rua {

class c_stream : public stream_base {
public:
	using native_handle_t = FILE *;

	constexpr c_stream(native_handle_t f = nullptr, bool need_close = true) :
		_f(f), $nc(need_close) {}

	c_stream(c_stream &&src) : c_stream(src._f, src.$nc) {
		src.detach();
	}

	RUA_OVERLOAD_ASSIGNMENT(c_stream)

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
		return _f && $nc;
	}

	virtual void close() {
		if (!_f) {
			return;
		}
		if ($nc) {
			fclose(_f);
		}
		_f = nullptr;
	}

	void detach() {
		$nc = false;
	}

private:
	native_handle_t _f;
	bool $nc;
};

} // namespace rua

#endif
