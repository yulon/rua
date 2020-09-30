#ifndef _RUA_STRING_CHAR_ENC_STREAM_UNI_HPP
#define _RUA_STRING_CHAR_ENC_STREAM_UNI_HPP

#include "../base/uni.hpp"

#include "../../../bytes.hpp"
#include "../../../io/util.hpp"

namespace rua { namespace uni {

class loc_to_u8_reader : public reader {
public:
	constexpr loc_to_u8_reader() : _lr(nullptr) {}

	loc_to_u8_reader(reader_i loc_reader) : _lr(std::move(loc_reader)) {}

	virtual ~loc_to_u8_reader() = default;

	virtual ptrdiff_t read(bytes_ref p) {
		return _lr->read(p);
	}

private:
	reader_i _lr;
};

class u8_to_loc_writer : public writer {
public:
	constexpr u8_to_loc_writer() : _lw(nullptr) {}

	u8_to_loc_writer(writer_i loc_writer) : _lw(std::move(loc_writer)) {}

	virtual ~u8_to_loc_writer() = default;

	virtual ptrdiff_t write(bytes_view p) {
		return _lw->write(p);
	}

private:
	writer_i _lw;
};

}} // namespace rua::uni

#endif
