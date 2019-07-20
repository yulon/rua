#ifndef _RUA_IO_ABSTRACT_HPP
#define _RUA_IO_ABSTRACT_HPP

#include "../bin.hpp"
#include "../any_ptr.hpp"
#include "../limits.hpp"
#include "../ref.hpp"

#include <cstddef>

namespace rua {

class reader {
public:
	virtual ~reader() = default;

	virtual size_t read(bin_ref) = 0;

	inline size_t read_full(bin_ref);

	inline bin read_all(size_t buf_grain_sz = 1024);
};

using reader_i = ref<reader>;

class writer {
public:
	virtual ~writer() = default;

	virtual size_t write(bin_view) = 0;

	inline bool write_all(bin_view);

	inline bool copy(const reader_i &r, bin_ref buf = nullptr);
};

using writer_i = ref<writer>;

class read_writer : public reader, public writer {};

using read_writer_i = ref<read_writer>;

class closer {
public:
	virtual ~closer() = default;

	virtual void close() = 0;
};

using closer_i = ref<closer>;

class read_closer : public reader, public closer {};

using read_closer_i = ref<read_closer>;

class write_closer : public writer, public closer {};

using write_closer_i = ref<write_closer>;

class read_write_closer : public read_writer, public closer {};

using read_write_closer_i = ref<read_write_closer>;

class seeker {
public:
	virtual ~seeker() = default;

	virtual size_t seek(size_t offset, int whence) = 0;
};

using seeker_i = ref<seeker>;

class read_seeker : public reader, public seeker {};

using read_seeker_i = ref<read_seeker>;

class write_seeker : public writer, public seeker {};

using write_seeker_i = ref<write_seeker>;

class read_write_seeker : public reader, public writer, public seeker {};

using read_write_seeker_i = ref<read_write_seeker>;

class reader_at {
public:
	virtual ~reader_at() = default;

	virtual size_t read_at(ptrdiff_t pos, bin_ref) = 0;
};

using reader_at_i = ref<reader_at>;

class writer_at {
public:
	virtual ~writer_at() = default;

	virtual size_t write_at(ptrdiff_t pos, bin_view) = 0;
};

using writer_at_i = ref<writer_at>;

class read_writer_at : public reader_at, public writer_at {};

using read_writer_at_i = ref<read_writer_at>;

class read_writer_at_closer : public read_writer_at, public closer {};

using read_writer_at_closer_i = ref<read_writer_at_closer>;

}

#endif
