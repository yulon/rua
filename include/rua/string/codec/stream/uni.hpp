#ifndef _rua_string_codec_stream_uni_hpp
#define _rua_string_codec_stream_uni_hpp

#include "../base/uni.hpp"

#include "../../../binary/bytes.hpp"
#include "../../../io/stream.hpp"

namespace rua { namespace uni {

namespace _string_codec_stream {

class l2u_reader : public stream {
public:
	constexpr l2u_reader() : $lr(nullptr) {}

	l2u_reader(stream_i loc_reader) : $lr(std::move(loc_reader)) {}

	virtual ~l2u_reader() {
		if (!$lr) {
			return;
		}
		$lr = nullptr;
	}

	operator bool() const override {
		return !!$lr;
	}

	future<readed_bytes> read(size_t n = 0) override {
		return $lr->read(n);
	}

private:
	stream_i $lr;
};

class u2l_writer : public stream {
public:
	constexpr u2l_writer() : $lw(nullptr) {}

	u2l_writer(stream_i loc_writer) : $lw(std::move(loc_writer)) {}

	virtual ~u2l_writer() {
		if (!$lw) {
			return;
		}
		$lw = nullptr;
	}

	explicit operator bool() const override {
		return !!$lw;
	}

	future<size_t> write(bytes_view p) override {
		return $lw->write(p);
	}

private:
	stream_i $lw;
};

} // namespace _string_codec_stream

using namespace _string_codec_stream;

}} // namespace rua::uni

#endif
