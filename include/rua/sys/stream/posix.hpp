#ifndef _rua_sys_stream_posix_hpp
#define _rua_sys_stream_posix_hpp

#include "../../io/util.hpp"
#include "../../util.hpp"

#include <unistd.h>

#include <cassert>

namespace rua { namespace posix {

class sys_stream : public stream {
public:
	using native_handle_t = int;

	constexpr sys_stream(native_handle_t fd = -1, bool need_close = true) :
		$fd(fd), $nc($fd >= 0 ? need_close : false) {}

	template <
		typename NullPtr,
		typename = enable_if_t<is_null_pointer<NullPtr>::value>>
	constexpr sys_stream(NullPtr) : sys_stream() {}

	sys_stream(const sys_stream &src) {
		if (!src.$fd) {
			$fd = -1;
			return;
		}
		if (!src.$nc) {
			$fd = src.$fd;
			$nc = false;
			return;
		}
		$fd = ::dup(src.$fd);
		assert($fd >= 0);
		$nc = true;
	}

	sys_stream(sys_stream &&src) : sys_stream(src.$fd, src.$nc) {
		src.detach();
	}

	RUA_OVERLOAD_ASSIGNMENT(sys_stream)

	virtual ~sys_stream() {
		close();
	}

	native_handle_t &native_handle() {
		return $fd;
	}

	native_handle_t native_handle() const {
		return $fd;
	}

	explicit operator bool() const override {
		return $fd >= 0;
	}

	future<size_t> read(bytes_ref buf) override {
		assert(*this);

		return $read($fd, buf);
	}

	future<size_t> write(bytes_view data) override {
		assert(*this);

		return $write($fd, data);
	}

	bool is_need_close() const {
		return $fd >= 0 && $nc;
	}

	void close() override {
		if ($fd < 0) {
			return;
		}
		if ($nc) {
			::close($fd);
		}
		$fd = -1;
	}

	void detach() {
		$nc = false;
	}

	sys_stream dup() const {
		if ($fd < 0) {
			return nullptr;
		}
		return ::dup($fd);
	}

private:
	int $fd;
	bool $nc;

	static size_t $read(int $fd, bytes_ref p) {
		return static_cast<size_t>(::read($fd, p.data(), p.size()));
	}

	static size_t $write(int $fd, bytes_view p) {
		return static_cast<size_t>(::write($fd, p.data(), p.size()));
	}
};

}} // namespace rua::posix

#endif
