#ifndef _RUA_STRING_CHAR_ENC_STREAM_UNI_HPP
#define _RUA_STRING_CHAR_ENC_STREAM_UNI_HPP

#include "../base/uni.hpp"

#include "../../../bytes.hpp"
#include "../../../io/stream.hpp"

namespace rua { namespace uni {

namespace _string_char_enc_stream {

class loc_to_u8_reader : public stream_base {
public:
	constexpr loc_to_u8_reader() : _lr(nullptr) {}

	loc_to_u8_reader(stream loc_reader) : _lr(std::move(loc_reader)) {}

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
		return _lr->read(p);
	}

private:
	stream _lr;
};

class u8_to_loc_writer : public stream_base {
public:
	constexpr u8_to_loc_writer() : _lw(nullptr) {}

	u8_to_loc_writer(stream loc_writer) : _lw(std::move(loc_writer)) {}

	virtual ~u8_to_loc_writer() {
		if (!_lw) {
			return;
		}
		_lw = nullptr;
	}

	virtual operator bool() const {
		return _lw;
	}

	virtual ssize_t write(bytes_view p) {
		return _lw->write(p);
	}

private:
	stream _lw;
};

} // namespace _string_char_enc_stream

using namespace _string_char_enc_stream;

}} // namespace rua::uni

#endif
