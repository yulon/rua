#ifndef _RUA_IO_UTIL_HPP
#define _RUA_IO_UTIL_HPP

#include "../macros.hpp"
#include "../string/conv.hpp"
#include "../sync/chan.hpp"
#include "../thread.hpp"
#include "../types/util.hpp"

#include <atomic>
#include <functional>
#include <vector>

namespace rua {

#ifndef RUA_IO_SIZE_DEFAULT
#define RUA_IO_SIZE_DEFAULT 4096
#endif

#ifndef RUA_LINE_SIZE_DEFAULT
#define RUA_LINE_SIZE_DEFAULT 1024
#endif

template <typename Reader>
class read_util {
public:
	// ptrdiff_t read(bytes_ref);

	///////////////////////////////////////////////////////

	bytes read_buffer;
	bytes_ref read_cache;

	ptrdiff_t get(bytes_ref buf) {
		if (!read_buffer) {
			return _this()->read(buf);
		}
		auto sz = _peek();
		if (sz <= 0) {
			return sz;
		}
		auto cp_sz = buf.copy(read_cache);
		read_cache = read_cache(cp_sz);
		return cp_sz;
	}

	bytes_ref get(size_t max_sz = nmax<size_t>()) {
		if (!read_buffer) {
			read_buffer.reset(RUA_IO_SIZE_DEFAULT);
		}
		auto sz = _peek();
		if (max_sz < sz) {
			auto got = read_cache(0, max_sz);
			read_cache = read_cache(max_sz);
			return got;
		}
		return std::move(read_cache);
	}

	bytes_ref peek(size_t max_sz = nmax<size_t>()) {
		if (!read_buffer) {
			read_buffer.reset(RUA_IO_SIZE_DEFAULT);
		}
		auto sz = _peek();
		if (max_sz < sz) {
			return read_cache(0, max_sz);
		}
		return read_cache;
	}

	bool discard(size_t sz) {
		if (!read_buffer) {
			read_buffer.reset(
				sz > RUA_IO_SIZE_DEFAULT ? RUA_IO_SIZE_DEFAULT : sz);
		}
		while (sz) {
			auto csz = _peek();
			if (csz <= 0) {
				return false;
			}
			if (sz < csz) {
				read_cache = read_cache(sz);
				return true;
			}
			sz -= csz;
			read_cache.reset();
		}
		return true;
	}

	ptrdiff_t read_full(bytes_ref buf) {
		auto full_sz = buf.size();
		size_t tsz = 0;
		while (tsz < full_sz) {
			auto sz = get(buf(tsz));
			if (sz <= 0) {
				return tsz ? tsz : sz;
			}
			tsz += sz;
		}
		return tsz;
	}

	bytes read_all(bytes buf = nullptr, size_t buf_grain_sz = 1024) {
		if (_discard_lf) {
			auto sz = _peek();
			if (!sz) {
				return nullptr;
			}
		}
		auto csz = read_cache.size();
		auto buf_init_sz = csz + buf_grain_sz;
		if (!buf) {
			buf.reset(buf_init_sz);
		}
		size_t tsz = 0;
		if (csz) {
			if (buf.size() < buf_init_sz) {
				buf.reset(buf_init_sz);
			}
			auto cp_sz = buf.copy(read_cache);
			if (cp_sz != csz) {
				buf.resize(0);
				return std::move(buf);
			}
			read_cache.reset();
			tsz += cp_sz;
		}
		for (;;) {
			auto sz = _this()->read(buf(tsz));
			if (!sz) {
				break;
			}
			tsz += static_cast<size_t>(sz);
			if (buf.size() - tsz < buf_grain_sz / 2) {
				buf.resize(buf.size() + buf_grain_sz);
				buf_grain_sz *= 2;
			}
		}
		buf.resize(tsz);
		return std::move(buf);
	}

	bytes read_all(size_t buf_grain_szf) {
		return read_all(nullptr, buf_grain_szf);
	}

	optional<std::string> get_line() {
		if (!read_buffer) {
			read_buffer.reset(RUA_LINE_SIZE_DEFAULT);
		}
		std::string ln;
		while (_peek() > 0) {
			for (ptrdiff_t i = 0; i < static_cast<ptrdiff_t>(read_cache.size());
				 ++i) {
				if (read_cache[i] == '\n') {
					ln += as_string(read_cache(0, i));
					read_cache = read_cache(i + 1);
					return {std::move(ln)};
				}
				if (read_cache[i] == '\r') {
					ln += as_string(read_cache(0, i));
					read_cache = read_cache(i + 1);
					if (read_cache.size() > 0 && read_cache[0] == '\n') {
						read_cache = read_cache(1);
					} else {
						_discard_lf = true;
					}
					return {std::move(ln)};
				}
			}
			ln += as_string(read_cache);
			read_cache.reset();
		}
		if (ln.length()) {
			return {std::move(ln)};
		}
		return nullopt;
	}

protected:
	constexpr read_util() : _discard_lf(false) {}

	~read_util() {
		if (_discard_lf) {
			_discard_lf = false;
		}
	}

private:
	Reader *_this() {
		return static_cast<Reader *>(this);
	}

	ptrdiff_t _peek() {
		for (;;) {
			if (read_cache) {
				return read_cache.size();
			}

			auto sz = _this()->read(read_buffer);
			if (sz <= 0) {
				return sz;
			}
			read_cache = read_buffer(0, sz);

			if (!_discard_lf) {
				return sz;
			}
			_discard_lf = false;

			if (read_buffer[0] != '\n') {
				return sz;
			}
			read_cache = read_cache(1);
		}
		return -211229;
	}

	bool _discard_lf;
};

template <typename Writer>
class write_util {
public:
	// ptrdiff_t write(bytes_view);

	///////////////////////////////////////////////////////

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
			auto sz = std::forward<Reader>(r).get(buf);
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

	///////////////////////////////////////////////////////

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
				auto sz = r.get(buf);
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

	operator bool() const {
		return _wa_li.size();
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
