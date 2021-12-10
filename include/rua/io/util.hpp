#ifndef _RUA_IO_UTIL_HPP
#define _RUA_IO_UTIL_HPP

#include "../macros.hpp"
#include "../sync/chan.hpp"
#include "../thread.hpp"
#include "../types/util.hpp"

#include <atomic>
#include <functional>
#include <vector>

namespace rua {

template <typename Reader>
class read_util {
public:
	// ptrdiff_t read(bytes_ref);

	ptrdiff_t read_full(bytes_ref p) {
		auto psz = static_cast<ptrdiff_t>(p.size());
		ptrdiff_t tsz = 0;
		while (tsz < psz) {
			auto sz = _this()->read(p(tsz));
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
			auto sz = _this()->read(buf(tsz));
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

protected:
	read_util() = default;

private:
	Reader *_this() {
		return static_cast<Reader *>(this);
	}
};

template <typename Writer>
class write_util {
public:
	// ptrdiff_t write(bytes_view);

	bool write_all(bytes_view p) {
		size_t tsz = 0;
		while (tsz < p.size()) {
			auto sz = _this()->write(p(tsz));
			if (!sz) {
				return false;
			}
			tsz += static_cast<size_t>(sz);
		}
		return true;
	}

	template <typename Reader>
	bool copy(Reader &&r, bytes_ref buf = nullptr) {
		bytes inner_buf;
		if (!buf) {
			inner_buf.reset(1024);
			buf = inner_buf;
		}
		for (;;) {
			auto sz = std::forward<Reader>(r).read(buf);
			if (sz <= 0) {
				return true;
			}
			if (!_this()->write_all(buf(0, sz))) {
				return false;
			}
		}
		return false;
	}

protected:
	write_util() = default;

private:
	Writer *_this() {
		return static_cast<Writer *>(this);
	}
};

template <typename ReadWriter>
class read_write_util : public read_util<ReadWriter>,
						public write_util<ReadWriter> {
protected:
	read_write_util() = default;
};

template <typename Seeker>
class seek_util {
public:
	// size_t seek(size_t offset, uchar whence);

	int64_t seek_to_begin(uint64_t offset = 0) {
		return _this()->seek(offset, 0);
	}

	int64_t seek_to_current(int64_t offset) {
		return _this()->seek(offset, 1);
	}

	int64_t seek_to_end(int64_t offset = 0) {
		return _this()->seek(offset, 2);
	}

protected:
	seek_util() = default;

private:
	Seeker *_this() {
		return static_cast<Seeker *>(this);
	}
};

template <typename ReadWriteSeeker>
class read_write_seek_util : public read_util<ReadWriteSeeker>,
							 public write_util<ReadWriteSeeker>,
							 public seek_util<ReadWriteSeeker> {
protected:
	read_write_seek_util() = default;
};

class read_group : public read_util<read_group> {
public:
	constexpr read_group(size_t buf_sz = 1024) : _c(0), _buf_sz(buf_sz) {}

	template <typename Reader>
	void add(Reader &r) {
		++_c;
		thread([this, &r]() {
			bytes buf(_buf_sz.load());
			for (;;) {
				auto sz = r.read(buf);
				if (sz <= 0) {
					_ch << nullptr;
					return;
				}
				_ch << bytes(buf(0, sz));
			}
		});
	}

	ptrdiff_t read(bytes_ref p) {
		while (!_buf) {
			_buf << _ch;
			if (!_buf) {
				if (--_c) {
					continue;
				}
				return 0;
			}
		}
		auto csz = static_cast<ptrdiff_t>(p.copy(_buf));
		_buf = _buf(csz);
		return csz;
	}

private:
	std::atomic<size_t> _c, _buf_sz;
	chan<bytes> _ch;
	bytes _buf;
};

class write_group : public write_util<write_group> {
public:
	write_group() = default;

	template <typename Writer>
	void add(Writer &w) {
		_wa_li.emplace_back([&w](bytes_view p) -> bool {
			return is_valid(w) && w.write_all(p);
		});
	}

	ptrdiff_t write(bytes_view p) {
		for (auto &wa : _wa_li) {
			wa(p);
		}
		return static_cast<ptrdiff_t>(p.size());
	}

private:
	std::vector<std::function<bool(bytes_view)>> _wa_li;
};

inline bool _is_stack_data(bytes_view data) {
	auto ptr = reinterpret_cast<uintptr_t>(data.data());
	auto end = reinterpret_cast<uintptr_t>(&data);
	auto begin = end - 2048;
	return begin < ptr && ptr < end;
}

inline bytes try_make_heap_data(bytes_view data) {
	return _is_stack_data(data) ? data : nullptr;
}

inline bytes try_make_heap_buffer(bytes_ref buf) {
	return _is_stack_data(buf) ? bytes(buf.size()) : nullptr;
}

} // namespace rua

#endif
