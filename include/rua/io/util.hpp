#ifndef _RUA_IO_UTIL_HPP
#define _RUA_IO_UTIL_HPP

#include "abstract.hpp"

#include "../sched/scheduler/this.hpp"
#include "../sync/chan.hpp"
#include "../thread.hpp"

#include <atomic>
#include <cstddef>
#include <initializer_list>
#include <vector>

namespace rua {

class read_group : public reader {
public:
	constexpr read_group(size_t buf_sz = 1024) : _c(0), _buf_sz(buf_sz) {}

	void add(reader_i r) {
		++_c;
		thread([this, r]() {
			bytes buf(_buf_sz.load());
			for (;;) {
				auto sz = r->read(buf);
				if (sz <= 0) {
					_ch << nullptr;
					return;
				}
				_ch << bytes(buf(0, sz));
			}
		});
	}

	virtual ptrdiff_t read(bytes_ref p) {
		while (!_buf) {
			_buf << _ch;
			if (!_buf) {
				if (--_c) {
					continue;
				}
				return 0;
			}
		}
		auto csz = static_cast<ptrdiff_t>(p.copy_from(_buf));
		_buf = _buf(csz);
		return csz;
	}

private:
	std::atomic<size_t> _c, _buf_sz;
	chan<bytes> _ch;
	bytes _buf;
};

class write_group : public writer {
public:
	write_group() = default;

	write_group(std::initializer_list<writer_i> li) : _li(li) {}

	void add(writer_i w) {
		_li.emplace_back(std::move(w));
	}

	virtual ptrdiff_t write(bytes_view p) {
		for (auto &w : _li) {
			w->write_all(p);
		}
		return static_cast<ptrdiff_t>(p.size());
	}

private:
	std::vector<writer_i> _li;
};

} // namespace rua

#endif
