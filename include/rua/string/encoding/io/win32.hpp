#ifndef _RUA_STRING_ENCODING_IO_WIN32_HPP
#define _RUA_STRING_ENCODING_IO_WIN32_HPP

#include "../base/win32.hpp"

#include "../../../bin.hpp"
#include "../../../io/util.hpp"

namespace rua { namespace win32 {

class loc_to_u8_reader : public virtual reader {
public:
	loc_to_u8_reader() : _lr(nullptr), _data_sz(0) {}

	loc_to_u8_reader(reader_i loc_reader) :
		_lr(std::move(loc_reader)),
		_data_sz(0) {}

	virtual ~loc_to_u8_reader() = default;

	virtual size_t read(bin_ref p) {
		while (_cache.empty()) {
			_buf.resize(_data_sz + p.size());

			auto rsz = _lr->read(_buf);
			if (!rsz) {
				if (_data_sz) {
					_cache = loc_to_u8(
						std::string(_buf.base().to<const char *>(), _data_sz));
				}
				break;
			}
			_data_sz += rsz;

			for (int i = static_cast<int>(_data_sz) - 1; i >= 0; ++i) {
				if (static_cast<char>(_buf[static_cast<size_t>(i)]) >= 0) {
					auto valid_sz = static_cast<size_t>(i + 1);

					_cache = loc_to_u8(
						std::string(_buf.base().to<const char *>(), valid_sz));

					_data_sz -= valid_sz;
					_buf.slice_self(valid_sz);
					break;
				}
			}
		};
		auto sz = p.copy(_cache);
		_cache = _cache.substr(sz, _cache.size() - sz);
		return sz;
	}

private:
	reader_i _lr;
	std::string _cache;
	bin _buf;
	size_t _data_sz;
};

class u8_to_loc_writer : public virtual writer {
public:
	u8_to_loc_writer() : _lw(nullptr) {}

	u8_to_loc_writer(writer_i loc_writer) : _lw(std::move(loc_writer)) {}

	virtual ~u8_to_loc_writer() = default;

	virtual size_t write(bin_view p) {
		_lw->write_all(
			u8_to_loc(std::string(p.base().to<const char *>(), p.size())));
		return p.size();
	}

private:
	writer_i _lw;
};

}} // namespace rua::win32

#endif
