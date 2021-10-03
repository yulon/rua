#ifndef _RUA_STRING_CHAR_ENC_STREAM_WIN32_HPP
#define _RUA_STRING_CHAR_ENC_STREAM_WIN32_HPP

#include "../base/win32.hpp"

#include "../../../bytes.hpp"
#include "../../../io/util.hpp"

namespace rua { namespace win32 {

namespace _string_char_enc_stream {

template <typename Reader>
class loc_to_u8_reader : public read_util<loc_to_u8_reader<Reader>> {
public:
	loc_to_u8_reader() : _lr(nullptr), _data_sz(0) {}

	loc_to_u8_reader(Reader &loc_reader) : _lr(&loc_reader), _data_sz(0) {}

	~loc_to_u8_reader() {
		if (!_lr) {
			return;
		}
		_lr = nullptr;
	}

	explicit operator bool() const {
		return is_valid(_lr);
	}

	virtual ptrdiff_t read(bytes_ref p) {
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
		auto sz = static_cast<ptrdiff_t>(p.copy_from(as_bytes(_cache)));
		_cache = _cache.substr(sz, _cache.size() - sz);
		return sz;
	}

private:
	Reader *_lr;
	std::string _cache;
	bytes _buf;
	ptrdiff_t _data_sz;
};

template <typename Reader>
loc_to_u8_reader<Reader> make_loc_to_u8_reader(Reader &loc_reader) {
	return loc_reader;
}

template <typename Writer>
class u8_to_loc_writer : public write_util<u8_to_loc_writer<Writer>> {
public:
	constexpr u8_to_loc_writer() : _lw(nullptr) {}

	u8_to_loc_writer(Writer &loc_writer) : _lw(&loc_writer) {}

	~u8_to_loc_writer() {
		if (!_lw) {
			return;
		}
		_lw = nullptr;
	}

	explicit operator bool() const {
		return is_valid(_lw);
	}

	ptrdiff_t write(bytes_view p) {
		_lw->write_all(as_bytes(u8_to_loc(as_string(p))));
		return static_cast<ptrdiff_t>(p.size());
	}

private:
	Writer *_lw;
};

template <typename Writer>
u8_to_loc_writer<Writer> make_u8_to_loc_writer(Writer &loc_writer) {
	return loc_writer;
}

} // namespace _string_char_enc_stream

using namespace _string_char_enc_stream;

}} // namespace rua::win32

#endif
