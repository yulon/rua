#ifndef _RUA_STRING_CHAR_ENC_STREAM_WIN32_HPP
#define _RUA_STRING_CHAR_ENC_STREAM_WIN32_HPP

#include "../base/win32.hpp"

#include "../../../bytes.hpp"
#include "../../../io/stream.hpp"

namespace rua { namespace win32 {

namespace _string_char_enc_stream {

class loc_to_u8_reader : public stream_base {
public:
	loc_to_u8_reader() : _lr(nullptr), _data_sz(0) {}

	loc_to_u8_reader(stream_i loc_reader) :
		_lr(std::move(loc_reader)), _data_sz(0) {}

	virtual ~loc_to_u8_reader() {
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
					_cache = loc_to_u8(as_string(_buf(0, _data_sz)));
				}
				break;
			}
			_data_sz += rsz;

			for (auto i = _data_sz - 1; i >= 0; ++i) {
				if (static_cast<char>(_buf[i]) >= 0) {
					auto valid_sz = i + 1;
					_cache = loc_to_u8(as_string(_buf(0, valid_sz)));
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

class u8_to_loc_writer : public stream_base {
public:
	constexpr u8_to_loc_writer() : _lw(nullptr) {}

	u8_to_loc_writer(stream_i loc_writer) : _lw(std::move(loc_writer)) {}

	virtual ~u8_to_loc_writer() {
		if (!_lw) {
			return;
		}
		_lw = nullptr;
	}

	explicit operator bool() const {
		return _lw;
	}

	virtual ssize_t write(bytes_view p) {
		_lw->write_all(as_bytes(u8_to_loc(as_string(p))));
		return to_signed(p.size());
	}

private:
	stream_i _lw;
};

} // namespace _string_char_enc_stream

using namespace _string_char_enc_stream;

}} // namespace rua::win32

#endif
