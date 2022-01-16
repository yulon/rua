#ifndef _RUA_IO_STREAM_HPP
#define _RUA_IO_STREAM_HPP

#include "../bytes.hpp"
#include "../interface_ptr.hpp"
#include "../types/util.hpp"

namespace rua {

class stream_base;

using stream_i = interface_ptr<stream_base>;

class stream_base {
public:
	virtual ~stream_base() = default;

	virtual operator bool() const {
		return true;
	}

	virtual ssize_t read(bytes_ref) {
		return 0;
	}

	ssize_t read_full(bytes_ref buf) {
		auto fsz = to_signed(buf.size());
		ssize_t tsz = 0;
		while (tsz < fsz) {
			auto sz = read(buf(tsz));
			if (sz <= 0) {
				return tsz ? tsz : sz;
			}
			tsz += sz;
		}
		return tsz;
	}

	bytes read_all(bytes &&buf = nullptr, size_t buf_alloc_sz = 1024) {
		if (!buf) {
			buf.resize(buf_alloc_sz);
		}
		size_t tsz = 0;
		for (;;) {
			auto sz = read(buf(tsz));
			if (!sz) {
				break;
			}
			tsz += static_cast<size_t>(sz);
			if (buf.size() - tsz < buf_alloc_sz / 2) {
				buf.resize(buf.size() + buf_alloc_sz);
				buf_alloc_sz *= 2;
			}
		}
		buf.resize(tsz);
		return std::move(buf);
	}

	virtual ssize_t write(bytes_view) {
		return 0;
	}

	ssize_t write_all(bytes_view p) {
		auto asz = to_signed(p.size());
		ssize_t tsz = 0;
		while (tsz < asz) {
			auto sz = write(p(tsz));
			if (!sz) {
				return tsz ? tsz : sz;
			}
			tsz += sz;
		}
		return tsz;
	}

	bool copy(stream_i r, bytes_ref buf = nullptr) {
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

	virtual int64_t seek(int64_t /* offset */, uchar /* whence */) {
		return 0;
	}

	int64_t seek_from_begin(int64_t offset = 0) {
		return seek(offset, 0);
	}

	int64_t seek_from_current(int64_t offset = 0) {
		return seek(offset, 1);
	}

	int64_t seek_from_end(int64_t offset = 0) {
		return seek(offset, 2);
	}

	virtual void close() {}

protected:
	stream_base() = default;
};

} // namespace rua

#endif
