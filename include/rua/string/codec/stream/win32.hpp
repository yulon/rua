#ifndef _rua_string_codec_stream_win32_hpp
#define _rua_string_codec_stream_win32_hpp

#include "../base/win32.hpp"

#include "../../../binary/bytes.hpp"
#include "../../../io/stream.hpp"

namespace rua { namespace win32 {

namespace _string_codec_stream {

class l2u_reader : public stream_base {
public:
	l2u_reader() : _lr(nullptr), _data_sz(0) {}

	l2u_reader(stream_i loc_reader) : _lr(std::move(loc_reader)), _data_sz(0) {}

	virtual ~l2u_reader() {
		if (!_lr) {
			return;
		}
		_lr = nullptr;
	}

	virtual operator bool() const {
		return _lr;
	}

	virtual ssize_t read(bytes_ref p) {
		while (_cache.empty()) {
			_buf.resize(_data_sz + p.size());

			auto rsz = _lr->read(_buf);
			if (rsz <= 0) {
				if (_data_sz) {
					_cache = l2u(as_string(_buf(0, _data_sz)));
				}
				break;
			}
			_data_sz += rsz;

			for (auto i = _data_sz - 1; i >= 0; ++i) {
				if (static_cast<char>(_buf[i]) >= 0) {
					auto valid_sz = i + 1;
					_cache = l2u(as_string(_buf(0, valid_sz)));
					_data_sz -= valid_sz;
					_buf = _buf(valid_sz);
					break;
				}
			}
		};
		auto sz = to_signed(p.copy(as_bytes(_cache)));
		_cache = _cache.substr(sz, _cache.size() - sz);
		return sz;
	}

private:
	stream_i _lr;
	std::string _cache;
	bytes _buf;
	ssize_t _data_sz;
};

class u2l_writer : public stream_base {
public:
	constexpr u2l_writer() : _lw(nullptr) {}

	u2l_writer(stream_i loc_writer) : _lw(std::move(loc_writer)) {}

	virtual ~u2l_writer() {
		if (!_lw) {
			return;
		}
		_lw = nullptr;
	}

	explicit operator bool() const {
		return _lw;
	}

	virtual ssize_t write(bytes_view p) {
		_lw->write_all(as_bytes(u2l(as_string(p))));
		return to_signed(p.size());
	}

private:
	stream_i _lw;
};

} // namespace _string_codec_stream

using namespace _string_codec_stream;

}} // namespace rua::win32

#endif
