#ifndef _RUA_STRING_CHAR_ENC_STREAM_UNI_HPP
#define _RUA_STRING_CHAR_ENC_STREAM_UNI_HPP

#include "../base/uni.hpp"

#include "../../../bytes.hpp"
#include "../../../io/util.hpp"

namespace rua { namespace uni {

namespace _string_char_enc_stream {

template <typename Reader>
class loc_to_u8_reader : public read_util<loc_to_u8_reader> {
public:
	constexpr loc_to_u8_reader() : _lr(nullptr) {}

	loc_to_u8_reader(Reader &loc_reader) : _lr(&loc_reader) {}

	~loc_to_u8_reader() {
		if (!_lr) {
			return;
		}
		_lr = nullptr;
	}

	explicit operator bool() const {
		return _lr && is_valid(*_lr);
	}

	ptrdiff_t read(bytes_ref p) {
		return _lr->read(p);
	}

private:
	Reader *_lr;
};

template <typename Reader>
loc_to_u8_reader<Reader> make_loc_to_u8_reader(Reader &loc_reader) {
	return loc_reader;
}

template <typename Writer>
class u8_to_loc_writer : public write_util<u8_to_loc_writer> {
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
		return _lw && is_valid(*_lw);
	}

	ptrdiff_t write(bytes_view p) {
		return _lw->write(p);
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

}} // namespace rua::uni

#endif
