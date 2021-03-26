#ifndef _RUA_IO_ABSTRACT_HPP
#define _RUA_IO_ABSTRACT_HPP

#include "../bytes.hpp"
#include "../interface_ptr.hpp"
#include "../types/util.hpp"

namespace rua {

class reader {
public:
	virtual ~reader() = default;

	virtual ptrdiff_t read(bytes_ref) = 0;

	ptrdiff_t read_full(bytes_ref p) {
		auto psz = static_cast<ptrdiff_t>(p.size());
		ptrdiff_t tsz = 0;
		while (tsz < psz) {
			auto sz = read(p(tsz));
			if (sz <= 0) {
				return tsz ? tsz : sz;
			}
			tsz += sz;
		}
		return tsz;
	}

	bytes read_all(size_t buf_grain_sz = 1024) {
		bytes buf(buf_grain_sz);
		size_t tsz = 0;
		for (;;) {
			auto sz = read(buf(tsz));
			if (!sz) {
				break;
			}
			tsz += static_cast<size_t>(sz);
			if (buf.size() - tsz < buf_grain_sz / 2) {
				buf.resize(buf.size() + buf_grain_sz);
			}
		}
		buf.resize(tsz);
		return buf;
	}
};

using reader_i = interface_ptr<reader>;

class writer {
public:
	virtual ~writer() = default;

	virtual ptrdiff_t write(bytes_view) = 0;

	bool write_all(bytes_view p) {
		size_t tsz = 0;
		while (tsz < p.size()) {
			auto sz = write(p(tsz));
			if (!sz) {
				return false;
			}
			tsz += static_cast<size_t>(sz);
		}
		return true;
	}

	bool copy(const reader_i &r, bytes_ref buf = nullptr) {
		bytes inner_buf;
		if (!buf) {
			inner_buf.reset(1024);
			buf = inner_buf;
		}
		for (;;) {
			auto sz = r->read(buf);
			if (sz <= 0) {
				return true;
			}
			if (!write_all(buf(0, sz))) {
				return false;
			}
		}
		return false;
	}
};

using writer_i = interface_ptr<writer>;

class read_writer : public reader, public writer {};

using read_writer_i = interface_ptr<read_writer>;

class closer {
public:
	virtual ~closer() = default;

	virtual void close() = 0;
};

using closer_i = interface_ptr<closer>;

class read_closer : public reader, public closer {};

using read_closer_i = interface_ptr<read_closer>;

class write_closer : public writer, public closer {};

using write_closer_i = interface_ptr<write_closer>;

class read_write_closer : public read_writer, public closer {};

using read_write_closer_i = interface_ptr<read_write_closer>;

class seeker {
public:
	virtual ~seeker() = default;

	virtual size_t seek(size_t offset, int whence) = 0;
};

using seeker_i = interface_ptr<seeker>;

class read_seeker : public reader, public seeker {};

using read_seeker_i = interface_ptr<read_seeker>;

class write_seeker : public writer, public seeker {};

using write_seeker_i = interface_ptr<write_seeker>;

class read_write_seeker : public reader, public writer, public seeker {};

using read_write_seeker_i = interface_ptr<read_write_seeker>;

class reader_at {
public:
	virtual ~reader_at() = default;

	virtual ptrdiff_t read_at(ptrdiff_t pos, bytes_ref) = 0;
};

using reader_at_i = interface_ptr<reader_at>;

class writer_at {
public:
	virtual ~writer_at() = default;

	virtual ptrdiff_t write_at(ptrdiff_t pos, bytes_view) = 0;
};

using writer_at_i = interface_ptr<writer_at>;

class read_writer_at : public reader_at, public writer_at {};

using read_writer_at_i = interface_ptr<read_writer_at>;

class read_writer_at_closer : public read_writer_at, public closer {};

using read_writer_at_closer_i = interface_ptr<read_writer_at_closer>;

} // namespace rua

#endif
