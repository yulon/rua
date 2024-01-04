#ifndef _rua_sys_stream_c_hpp
#define _rua_sys_stream_c_hpp

#include "../../io/stream.hpp"
#include "../../util.hpp"

#include <cstddef>
#include <cstdio>

namespace rua {

class c_stream : public stream {
public:
	using native_handle_t = FILE *;

	constexpr c_stream(native_handle_t f = nullptr, bool need_close = true) :
		$f(f), $nc(need_close) {}

	c_stream(c_stream &&src) : c_stream(src.$f, src.$nc) {
		src.detach();
	}

	RUA_OVERLOAD_ASSIGNMENT(c_stream)

	virtual ~c_stream() {
		close();
	}

	native_handle_t &native_handle() {
		return $f;
	}

	native_handle_t native_handle() const {
		return $f;
	}

	explicit operator bool() const override {
		return $f;
	}

	bool is_need_close() const {
		return $f && $nc;
	}

	future<> close() override {
		auto $ = self().as<c_stream>();
		return stream::close() >> [$]() {
			if (!$->$f) {
				return;
			}
			if ($->$nc) {
				fclose($->$f);
			}
			$->$f = nullptr;
		};
	}

	void detach() {
		$nc = false;
	}

protected:
	expected<size_t> unbuf_sync_read(bytes_ref p) override {
		return static_cast<size_t>(fread(p.data(), 1, p.size(), $f));
	}

	expected<size_t> unbuf_sync_write(bytes_view p) override {
		return static_cast<size_t>(fwrite(p.data(), 1, p.size(), $f));
	}

private:
	native_handle_t $f;
	bool $nc;
};

} // namespace rua

#endif
