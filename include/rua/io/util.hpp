#ifndef _rua_io_util_hpp
#define _rua_io_util_hpp

#include "./stream.hpp"

#include "../string/conv.hpp"
#include "../sync/chan.hpp"
#include "../thread.hpp"
#include "../util.hpp"

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
		$r(std::move(r)), $r_buf(std::move(buf)), $skip_lf(false) {}

	buffered_reader(stream_i r, size_t buf_sz) :
		buffered_reader(std::move(r), bytes(buf_sz)) {}

	virtual ~buffered_reader() = default;

	virtual operator bool() const {
		return !!$r;
	}

	virtual ssize_t read(bytes_ref buf) {
		if (!$r_buf) {
			return $r->read(buf);
		}
		auto csz = $peek();
		if (csz <= 0) {
			return csz;
		}
		auto cp_sz = buf.copy($r_cache);
		$r_cache = $r_cache(cp_sz);
		return cp_sz;
	}

	bytes_ref peek(size_t max_sz = nmax<size_t>()) {
		if (!$r_buf) {
			$r_buf.reset(RUA_IO_SIZE_DEFAULT);
		}
		auto csz = $peek();
		if (csz <= 0) {
			return $r_cache(0, max_sz);
		}
		if (max_sz < to_unsigned(csz)) {
			return $r_cache(0, max_sz);
		}
		return $r_cache;
	}

	bool discard(size_t sz) {
		if (!$r_buf) {
			$r_buf.reset(sz > RUA_IO_SIZE_DEFAULT ? RUA_IO_SIZE_DEFAULT : sz);
		}
		while (sz) {
			auto csz = $peek();
			if (csz <= 0) {
				return false;
			}
			auto cusz = to_unsigned(csz);
			if (sz < cusz) {
				$r_cache = $r_cache(sz);
				return true;
			}
			sz -= cusz;
			$r_cache.reset();
		}
		return true;
	}

	bytes read_all(bytes &&buf = nullptr, size_t buf_alloc_sz = 1024) {
		if ($skip_lf) {
			auto csz = $peek();
			if (csz <= 0) {
				return nullptr;
			}
		}
		auto csz = $r_cache.size();
		auto buf_init_sz = csz + buf_alloc_sz;
		if (!buf) {
			buf.reset(buf_init_sz);
		}
		size_t tsz = 0;
		if (csz) {
			if (buf.size() < buf_init_sz) {
				buf.reset(buf_init_sz);
			}
			auto cp_sz = buf.copy($r_cache);
			if (cp_sz != csz) {
				buf.resize(0);
				return std::move(buf);
			}
			$r_cache.reset();
			tsz += cp_sz;
		}
		for (;;) {
			auto sz = $r->read(buf(tsz));
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
		if (!$r_buf) {
			$r_buf.reset(RUA_LINE_SIZE_DEFAULT);
		}

		if ($ln_buf_opt) {
			$ln_buf_opt->resize(0);
		} else {
			$ln_buf_opt.emplace();
		}
		auto &ln_buf = *$ln_buf_opt;

		while ($peek() > 0) {
			for (ssize_t i = 0; i < to_signed($r_cache.size()); ++i) {
				if ($r_cache[i] == '\n') {
					ln_buf += as_string($r_cache(0, i));
					$r_cache = $r_cache(i + 1);
					return {ln_buf};
				}
				if ($r_cache[i] == '\r') {
					ln_buf += as_string($r_cache(0, i));
					$r_cache = $r_cache(i + 1);
					if ($r_cache.size() > 0 && $r_cache[0] == '\n') {
						$r_cache = $r_cache(1);
					} else {
						$skip_lf = true;
					}
					return {ln_buf};
				}
			}
			ln_buf += as_string($r_cache);
			$r_cache.reset();
		}
		if (ln_buf.length()) {
			return {ln_buf};
		}
		return nullopt;
	}

private:
	stream_i $r;
	bytes $r_buf;
	bytes_ref $r_cache;
	optional<std::string> $ln_buf_opt;
	bool $skip_lf;

	ssize_t $peek() {
		for (;;) {
			if ($r_cache) {
				return $r_cache.size();
			}

			auto sz = $r->read($r_buf);
			if (sz <= 0) {
				return sz;
			}
			$r_cache = $r_buf(0, sz);

			if (!$skip_lf) {
				return sz;
			}
			$skip_lf = false;

			if ($r_buf[0] != '\n') {
				return sz;
			}
			$r_cache = $r_cache(1);
		}
		return -211229;
	}
};

class buffered_writer : public stream_base {
public:
	constexpr buffered_writer() = default;

	buffered_writer(stream_i w, bytes &&w_buf = nullptr) :
		$w(std::move(w)), $w_buf(std::move(w_buf)) {}

	buffered_writer(stream_i w, size_t buf_sz) :
		buffered_writer(std::move(w), bytes(buf_sz)) {}

	virtual ~buffered_writer() = default;

	virtual operator bool() const {
		return !!$w;
	}

	virtual ssize_t write(bytes_view data) {
		if (!$w_buf) {
			return $w->write(data);
		}
		auto dsz = to_signed(data.size());
		ssize_t tsz = 0;
		while (tsz < dsz) {
			assert($w_cache.size() < $w_buf.size());
			auto sz = $w_buf($w_cache.size()).copy(data(tsz));
			if (!sz) {
				continue;
			}
			tsz += sz;
			$w_cache = $w_buf($w_cache.size() + sz);
			if ($w_cache.size() == $w_buf.size() && !$flush()) {
				return tsz;
			}
		}
		return tsz;
	}

	bool flush() {
		if (!$w_cache) {
			return true;
		}
		return $flush();
	}

private:
	stream_i $w;
	bytes $w_buf;
	bytes_ref $w_cache;

	bool $flush() {
		auto sz = write_all($w_cache);
		$w_cache = $w_cache(sz);
		if ($w_cache) {
			return false;
		}
		return true;
	}
};

class read_group : public stream_base {
public:
	constexpr read_group(size_t buf_sz = 1024) : _c(0), $buf_sz(buf_sz) {}

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
			bytes buf($buf_sz.load());
			for (;;) {
				auto sz = r->read(buf);
				if (sz <= 0) {
					$ch.send(nullptr);
					return;
				}
				$ch.send(bytes(buf(0, sz)));
			}
		});
	}

	virtual ssize_t read(bytes_ref buf) {
		while (!$buf) {
			$buf = **$ch.recv();
			if (!$buf) {
				if (--_c) {
					continue;
				}
				return 0;
			}
		}
		auto csz = to_signed(buf.copy($buf));
		$buf = $buf(csz);
		return csz;
	}

	virtual operator bool() const {
		return _c.load();
	}

private:
	std::atomic<size_t> _c, $buf_sz;
	chan<bytes> $ch;
	bytes $buf;
};

class write_group : public stream_base {
public:
	write_group() = default;

	write_group(std::vector<stream_i> w_li) : $w_li(std::move(w_li)) {}

	virtual ~write_group() = default;

	void add(stream_i w) {
		$w_li.emplace_back(std::move(w));
	}

	virtual ssize_t write(bytes_view data) {
		for (auto &w : $w_li) {
			if (w) {
				w->write_all(data);
			}
		}
		return to_signed(data.size());
	}

	virtual operator bool() const {
		return $w_li.size();
	}

private:
	std::vector<stream_i> $w_li;
};

} // namespace rua

#endif
