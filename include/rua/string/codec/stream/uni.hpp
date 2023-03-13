#ifndef _rua_string_codec_stream_uni_hpp
#define _rua_string_codec_stream_uni_hpp

#include "../base/uni.hpp"

#include "../../../binary/bytes.hpp"
#include "../../../io/stream.hpp"

namespace rua { namespace uni {

namespace _string_codec_stream {

class l2u_reader : public stream_base {
public:
	constexpr l2u_reader() : $lr(nullptr) {}

	l2u_reader(stream_i loc_reader) : $lr(std::move(loc_reader)) {}

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
		return $lr->read(p);
	}

private:
	stream_i $lr;
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

	virtual operator bool() const {
		return $lw;
	}

	virtual ssize_t write(bytes_view p) {
		return $lw->write(p);
	}

private:
	stream_i $lw;
};

} // namespace _string_codec_stream

using namespace _string_codec_stream;

}} // namespace rua::uni

#endif
