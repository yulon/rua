#ifndef _RUA_IO_UTIL_HPP
#define _RUA_IO_UTIL_HPP

#include "./stream.hpp"

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

class buffered_reader : public stream_base {
public:
	buffered_reader() = default;

	buffered_reader(stream_i r, bytes &&buf = nullptr) :
		_r(std::move(r)), _r_buf(std::move(buf)), _skip_lf(false) {}

	buffered_reader(stream_i r, size_t buf_sz) :
		buffered_reader(std::move(r), bytes(buf_sz)) {}

	virtual ~buffered_reader() = default;

	virtual operator bool() const {
		return _r;
	}

	virtual ssize_t read(bytes_ref buf) {
		if (!_r_buf) {
			return _r->read(buf);
		}
		auto csz = _peek();
		if (csz <= 0) {
			return csz;
		}
		auto cp_sz = buf.copy(_r_cache);
		_r_cache = _r_cache(cp_sz);
		return cp_sz;
	}

	bytes_ref peek(size_t max_sz = nmax<size_t>()) {
		if (!_r_buf) {
			_r_buf.reset(RUA_IO_SIZE_DEFAULT);
		}
		auto csz = _peek();
		if (csz <= 0) {
			return _r_cache(0, max_sz);
		}
		if (max_sz < to_unsigned(csz)) {
			return _r_cache(0, max_sz);
		}
		return _r_cache;
	}

	bool discard(size_t sz) {
		if (!_r_buf) {
			_r_buf.reset(sz > RUA_IO_SIZE_DEFAULT ? RUA_IO_SIZE_DEFAULT : sz);
		}
		while (sz) {
			auto csz = _peek();
			if (csz <= 0) {
				return false;
			}
			auto cusz = to_unsigned(csz);
			if (sz < cusz) {
				_r_cache = _r_cache(sz);
				return true;
			}
			sz -= cusz;
			_r_cache.reset();
		}
		return true;
	}

	bytes read_all(bytes &&buf = nullptr, size_t buf_alloc_sz = 1024) {
		if (_skip_lf) {
			auto csz = _peek();
			if (csz <= 0) {
				return nullptr;
			}
		}
		auto csz = _r_cache.size();
		auto buf_init_sz = csz + buf_alloc_sz;
		if (!buf) {
			buf.reset(buf_init_sz);
		}
		size_t tsz = 0;
		if (csz) {
			if (buf.size() < buf_init_sz) {
				buf.reset(buf_init_sz);
			}
			auto cp_sz = buf.copy(_r_cache);
			if (cp_sz != csz) {
				buf.resize(0);
				return std::move(buf);
			}
			_r_cache.reset();
			tsz += cp_sz;
		}
		for (;;) {
			auto sz = _r->read(buf(tsz));
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

	// TODO: std::pair<std::string, error> read_line(std::string &&)

	optional<std::string> read_line() {
		if (!_r_buf) {
			_r_buf.reset(RUA_LINE_SIZE_DEFAULT);
		}

		if (_ln_buf_opt) {
			_ln_buf_opt->resize(0);
		} else {
			_ln_buf_opt.emplace();
		}
		auto &ln_buf = *_ln_buf_opt;

		while (_peek() > 0) {
			for (ssize_t i = 0; i < to_signed(_r_cache.size()); ++i) {
				if (_r_cache[i] == '\n') {
					ln_buf += as_string(_r_cache(0, i));
					_r_cache = _r_cache(i + 1);
					return {ln_buf};
				}
				if (_r_cache[i] == '\r') {
					ln_buf += as_string(_r_cache(0, i));
					_r_cache = _r_cache(i + 1);
					if (_r_cache.size() > 0 && _r_cache[0] == '\n') {
						_r_cache = _r_cache(1);
					} else {
						_skip_lf = true;
					}
					return {ln_buf};
				}
			}
			ln_buf += as_string(_r_cache);
			_r_cache.reset();
		}
		if (ln_buf.length()) {
			return {ln_buf};
		}
		return nullopt;
	}

private:
	stream_i _r;
	bytes _r_buf;
	bytes_ref _r_cache;
	optional<std::string> _ln_buf_opt;
	bool _skip_lf;

	ssize_t _peek() {
		for (;;) {
			if (_r_cache) {
				return _r_cache.size();
			}

			auto sz = _r->read(_r_buf);
			if (sz <= 0) {
				return sz;
			}
			_r_cache = _r_buf(0, sz);

			if (!_skip_lf) {
				return sz;
			}
			_skip_lf = false;

			if (_r_buf[0] != '\n') {
				return sz;
			}
			_r_cache = _r_cache(1);
		}
		return -211229;
	}
};

class buffered_writer : public stream_base {
public:
	constexpr buffered_writer() = default;

	buffered_writer(stream_i w, bytes &&w_buf = nullptr) :
		_w(std::move(w)), _w_buf(std::move(w_buf)) {}

	buffered_writer(stream_i w, size_t buf_sz) :
		buffered_writer(std::move(w), bytes(buf_sz)) {}

	virtual ~buffered_writer() = default;

	virtual operator bool() const {
		return _w;
	}

	virtual ssize_t write(bytes_view data) {
		if (!_w_buf) {
			return _w->write(data);
		}
		auto dsz = to_signed(data.size());
		ssize_t tsz = 0;
		while (tsz < dsz) {
			assert(_w_cache.size() < _w_buf.size());
			auto sz = _w_buf(_w_cache.size()).copy(data(tsz));
			if (!sz) {
				continue;
			}
			tsz += sz;
			_w_cache = _w_buf(_w_cache.size() + sz);
			if (_w_cache.size() == _w_buf.size() && !_flush()) {
				return tsz;
			}
		}
		return tsz;
	}

	bool flush() {
		if (!_w_cache) {
			return true;
		}
		return _flush();
	}

private:
	stream_i _w;
	bytes _w_buf;
	bytes_ref _w_cache;

	bool _flush() {
		auto sz = write_all(_w_cache);
		_w_cache = _w_cache(sz);
		if (_w_cache) {
			return false;
		}
		return true;
	}
};

class read_group : public stream_base {
public:
	constexpr read_group(size_t buf_sz = 1024) : _c(0), _buf_sz(buf_sz) {}

	read_group(std::initializer_list<stream_i> r_li, size_t buf_sz = 1024) :
		read_group(buf_sz) {
		for (auto &r : r_li) {
			add(std::move(r));
		}
	}

	virtual ~read_group() = default;

	void add(stream_i r) {
		++_c;
		thread([this, r]() {
			bytes buf(_buf_sz.load());
			for (;;) {
				auto sz = r->read(buf);
				if (sz <= 0) {
					_ch.send(nullptr);
					return;
				}
				_ch.send(bytes(buf(0, sz)));
			}
		});
	}

	virtual ssize_t read(bytes_ref buf) {
		while (!_buf) {
			_buf = *_ch.recv();
			if (!_buf) {
				if (--_c) {
					continue;
				}
				return 0;
			}
		}
		auto csz = to_signed(buf.copy(_buf));
		_buf = _buf(csz);
		return csz;
	}

	virtual operator bool() const {
		return _c.load();
	}

private:
	std::atomic<size_t> _c, _buf_sz;
	chan<bytes> _ch;
	bytes _buf;
};

class write_group : public stream_base {
public:
	write_group() = default;

	write_group(std::vector<stream_i> w_li) : _w_li(std::move(w_li)) {}

	virtual ~write_group() = default;

	void add(stream_i w) {
		_w_li.emplace_back(std::move(w));
	}

	virtual ssize_t write(bytes_view data) {
		for (auto &w : _w_li) {
			if (w) {
				w->write_all(data);
			}
		}
		return to_signed(data.size());
	}

	virtual operator bool() const {
		return _w_li.size();
	}

private:
	std::vector<stream_i> _w_li;
};

} // namespace rua

#endif
