#ifndef _RUA_STRING_CHAR_CODEC_STREAM_UNI_HPP
#define _RUA_STRING_CHAR_CODEC_STREAM_UNI_HPP

#include "../base/uni.hpp"

#include "../../../bytes.hpp"
#include "../../../io/stream.hpp"

namespace rua { namespace uni {

namespace _string_char_codec_stream {

class l2u_reader : public stream_base {
public:
	constexpr l2u_reader() : _lr(nullptr) {}

	l2u_reader(stream_i loc_reader) : _lr(std::move(loc_reader)) {}

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
		return _lr->read(p);
	}

private:
	stream_i _lr;
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

	virtual operator bool() const {
		return _lw;
	}

	virtual ssize_t write(bytes_view p) {
		return _lw->write(p);
	}

private:
	stream_i _lw;
};

} // namespace _string_char_codec_stream

using namespace _string_char_codec_stream;

}} // namespace rua::uni

#endif
