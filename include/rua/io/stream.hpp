#ifndef _rua_io_stream_hpp
#define _rua_io_stream_hpp

#include "../binary/bytes.hpp"
#include "../conc.hpp"
#include "../dype/interface_ptr.hpp"
#include "../error.hpp"
#include "../move_only.hpp"
#include "../thread.hpp"
#include "../util.hpp"

#include <cassert>

namespace rua {

RUA_CVAR const strv_error err_stream_was_closed("stream was closed");

class stream;

using stream_i = interface_ptr<stream>;

class readed_bytes : public bytes_view {
public:
	constexpr readed_bytes() = default;

	readed_bytes(stream_i strm, bytes_view data) :
		bytes_view(data), $strm(std::move(strm)) {}

	const stream_i &strm() const & {
		return $strm;
	}

	stream_i &&strm() && {
		return std::move($strm);
	}

private:
	stream_i $strm;
};

class stream {
public:
	virtual ~stream() = default;

	virtual explicit operator bool() const {
		return true;
	}

	virtual future<readed_bytes> peek(size_t n = 0) {
		if (!*this) {
			return err_stream_was_closed;
		}

		auto $ = self();

		auto r_cac = $r_cac();
		if (n) {
			if (r_cac.size() >= n) {
				return readed_bytes($, $r_buf($r_cac_start, $r_cac_start + n));
			}
			if ($r_buf.size() - $r_cac_start < n) {
				$r_buf.resize((($r_cac_start + n - 1) / 1024 + 1) * 1024);
			}
		} else {
			if (r_cac.size()) {
				return readed_bytes($, r_cac);
			}
			if (!$r_buf) {
				$r_buf.reset(4096);
			}
		}

		return unbuf_async_read($r_buf($r_cac_end)) >> [=](size_t rn) mutable {
			$->$r_cac_end += rn;
			return $->peek(n);
		};
	}

	virtual future<readed_bytes> read(size_t n = 0) {
		if (!*this) {
			return err_stream_was_closed;
		}

		return peek(n) >> [this](readed_bytes data) mutable {
			assert(data.size());

			assert($r_cac_start < $r_cac_end);
			$r_cac_start += data.size();

			assert($r_cac_end >= $r_cac_start);
			if ($r_cac_end == $r_cac_start) {
				$r_cac_start = 0;
				$r_cac_end = 0;
			}

			return data;
		};
	}

	virtual future<> flush() {
		if (!*this) {
			return err_stream_was_closed;
		}

		auto $ = self();

		auto w_cac = $w_cac();
		if (!w_cac.size()) {
			return meet_expected;
		}

		return unbuf_async_write(w_cac) >> [=](size_t wn) mutable -> future<> {
			assert(wn);

			assert($->$w_cac_start < $->$w_cac_end);
			$->$w_cac_start += wn;

			assert($->$w_cac_start <= $->$w_cac_end);
			if ($->$w_cac_start == $->$w_cac_end) {
				$->$w_cac_start = 0;
				$->$w_cac_end = 0;
				if ($->$w_buf.size() > 4096) {
					$->$w_buf.resize(4096);
				}
				return meet_expected;
			}

			return $->flush();
		};
	}

	virtual future<size_t> write(bytes_view data) {
		if (!*this) {
			return err_stream_was_closed;
		}

		assert($w_cac_start == 0);
		assert($w_cac_end <= $w_buf.size());

		auto $ = self();

		if (!$w_buf) {
			$w_buf.reset(4096);
		}

		auto w_buf_tail_n = $w_buf.size() - $w_cac_end;
		auto data_n = data.size();
		auto no_flush = w_buf_tail_n > data_n;

		if (data_n > w_buf_tail_n) {
			$w_buf.resize((($w_cac_end + data_n - 1) / 1024 + 1) * 1024);
			assert($w_buf.size() > 4096);
		}

		auto cp_n = $w_buf($w_cac_end).copy(data);
		assert(cp_n == data_n);
		$w_cac_end += cp_n;

		if (no_flush) {
			return cp_n;
		}

		return flush() >> [=]() -> future<size_t> {
			assert($->$w_cac_start == 0);
			assert($->$w_cac_end == 0);

			return data_n;
		};
	}

	future<> write_all(bytes_view data) {
		auto $ = self();
		return write(data) >> [=]() { return $->flush(); };
	}

	future<> copy(stream_i r) {
		return meet_expected;
	}

	virtual int64_t seek(int64_t /* offset */, uchar /* whence */) {
		if (!*this) {
			return 0;
		}

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

	virtual future<> close() {
		if (!*this) {
			return err_stream_was_closed;
		}
		return flush();
	}

protected:
	constexpr stream() :
		$r_buf(),
		$w_buf(),
		$r_cac_start(0),
		$r_cac_end(0),
		$w_cac_start(0),
		$w_cac_end(0) {}

	virtual expected<size_t> unbuf_sync_read(bytes_ref) {
		return err_unimplemented;
	}

	virtual expected<size_t> unbuf_sync_write(bytes_view) {
		return err_unimplemented;
	}

	virtual future<size_t> unbuf_async_read(bytes_ref buf) {
		auto $ = self();
		return parallel(
			[=]() -> rua::expected<size_t> { return $->unbuf_sync_read(buf); });
	}

	virtual future<size_t> unbuf_async_write(bytes_view data) {
		auto $ = self();
		return parallel([=]() -> rua::expected<size_t> {
			return $->unbuf_sync_write(data);
		});
	}

	virtual stream_i self() {
		return this;
	}

private:
	bytes $r_buf, $w_buf;
	size_t $r_cac_start, $r_cac_end, $w_cac_start, $w_cac_end;

	bytes_view $r_cac() {
		return $r_buf($r_cac_start, $r_cac_end);
	}

	bytes_view $w_cac() {
		return $w_buf($w_cac_start, $w_cac_end);
	}
};

} // namespace rua

#endif
