#ifndef _RUA_STRING_ENCODING_STREAM_WIN32_HPP
#define _RUA_STRING_ENCODING_STREAM_WIN32_HPP

#include "../base/win32.hpp"

#include "../../../bytes.hpp"
#include "../../../io/util.hpp"

namespace rua { namespace win32 {

class loc_to_u8_reader : public reader {
public:
	loc_to_u8_reader() : _lr(nullptr), _data_sz(0) {}

	loc_to_u8_reader(reader_i loc_reader) :
		_lr(std::move(loc_reader)), _data_sz(0) {}

	virtual ~loc_to_u8_reader() = default;

	virtual ptrdiff_t read(bytes_ref p) {
		while (_cache.empty()) {
			_buf.resize(_data_sz + p.size());

			auto rsz = _lr->read(_buf);
			if (rsz <= 0) {
				if (_data_sz) {
					_cache = loc_to_u8(_buf(0, _data_sz));
				}
				break;
			}
			_data_sz += rsz;

			for (auto i = _data_sz - 1; i >= 0; ++i) {
				if (static_cast<char>(_buf[i]) >= 0) {
					auto valid_sz = i + 1;
					_cache = loc_to_u8(_buf(0, valid_sz));
					_data_sz -= valid_sz;
					_buf = _buf(valid_sz);
					break;
				}
			}
		};
		auto sz = static_cast<ptrdiff_t>(p.copy_from(_cache));
		_cache = _cache.substr(sz, _cache.size() - sz);
		return sz;
	}

private:
	reader_i _lr;
	std::string _cache;
	bytes _buf;
	ptrdiff_t _data_sz;
};

class u8_to_loc_writer : public writer {
public:
	constexpr u8_to_loc_writer() : _lw(nullptr) {}

	u8_to_loc_writer(writer_i loc_writer) : _lw(std::move(loc_writer)) {}

	virtual ~u8_to_loc_writer() = default;

	virtual ptrdiff_t write(bytes_view p) {
		_lw->write_all(u8_to_loc(p));
		return static_cast<ptrdiff_t>(p.size());
	}

private:
	writer_i _lw;
};

}} // namespace rua::win32

#endif
