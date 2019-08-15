#ifndef _RUA_STRING_ENCODING_IO_UNI_HPP
#define _RUA_STRING_ENCODING_IO_UNI_HPP

#include "../base/uni.hpp"

#include "../../../bytes.hpp"
#include "../../../io/util.hpp"

namespace rua { namespace uni {

class loc_to_u8_reader : public virtual reader {
public:
	loc_to_u8_reader() : _lr(nullptr) {}

	loc_to_u8_reader(reader_i loc_reader) : _lr(std::move(loc_reader)) {}

	virtual ~loc_to_u8_reader() = default;

	virtual size_t read(bytes_ref p) {
		return _lr->read(p);
	}

private:
	reader_i _lr;
};

class u8_to_loc_writer : public virtual writer {
public:
	u8_to_loc_writer() : _lw(nullptr) {}

	u8_to_loc_writer(writer_i loc_writer) : _lw(std::move(loc_writer)) {}

	virtual ~u8_to_loc_writer() = default;

	virtual size_t write(bytes_view p) {
		return _lw->write(p);
	}

private:
	writer_i _lw;
};

}} // namespace rua::uni

#endif
