#ifndef _rua_string_codec_stream_win32_hpp
#define _rua_string_codec_stream_win32_hpp

#include "../base/win32.hpp"

#include "../../../binary/bytes.hpp"
#include "../../../io/stream.hpp"

namespace rua { namespace win32 {

namespace _string_codec_stream {

class l2u_reader : public stream_base {
public:
	l2u_reader() : $lr(nullptr), $data_sz(0) {}

	l2u_reader(stream_i loc_reader) : $lr(std::move(loc_reader)), $data_sz(0) {}

	virtual ~l2u_reader() {
		if (!$lr) {
			return;
		}
		$lr = nullptr;
	}

	virtual operator bool() const {
		return $lr;
	}

	virtual ssize_t read(bytes_ref p) {
		while ($cache.empty()) {
			$buf.resize($data_sz + p.size());

			auto rsz = $lr->read($buf);
			if (rsz <= 0) {
				if ($data_sz) {
					$cache = l2u(as_string($buf(0, $data_sz)));
				}
				break;
			}
			$data_sz += rsz;

			for (auto i = $data_sz - 1; i >= 0; ++i) {
				if (static_cast<char>($buf[i]) >= 0) {
					auto valid_sz = i + 1;
					$cache = l2u(as_string($buf(0, valid_sz)));
					$data_sz -= valid_sz;
					$buf = $buf(valid_sz);
					break;
				}
			}
		};
		auto sz = to_signed(p.copy(as_bytes($cache)));
		$cache = $cache.substr(sz, $cache.size() - sz);
		return sz;
	}

private:
	stream_i $lr;
	std::string $cache;
	bytes $buf;
	ssize_t $data_sz;
};

class u2l_writer : public stream_base {
public:
	constexpr u2l_writer() : $lw(nullptr) {}

	u2l_writer(stream_i loc_writer) : $lw(std::move(loc_writer)) {}

	virtual ~u2l_writer() {
		if (!$lw) {
			return;
		}
		$lw = nullptr;
	}

	explicit operator bool() const {
		return $lw;
	}

	virtual ssize_t write(bytes_view p) {
		$lw->write_all(as_bytes(u2l(as_string(p))));
		return to_signed(p.size());
	}

private:
	stream_i $lw;
};

} // namespace _string_codec_stream

using namespace _string_codec_stream;

}} // namespace rua::win32

#endif
