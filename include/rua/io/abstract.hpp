#ifndef _RUA_IO_ABSTRACT_HPP
#define _RUA_IO_ABSTRACT_HPP

#include "../bin.hpp"
#include "../any_ptr.hpp"
#include "../limits.hpp"
#include "../interface_ptr.hpp"

#include <cstddef>

namespace rua {
namespace io {

class reader {
public:
	virtual ~reader() = default;

	virtual size_t read(bin_ref) = 0;

	inline size_t read_full(bin_ref);

	inline bin read_all(size_t buf_grain_sz = 1024);
};

using reader_i = interface_ptr<reader>;

class writer {
public:
	virtual ~writer() = default;

	virtual size_t write(bin_view) = 0;

	inline bool write_all(bin_view);

	inline bool copy(const reader_i &r, bin_ref buf = nullptr);
};

using writer_i = interface_ptr<writer>;

class read_writer : public virtual reader, public virtual writer {};

using read_writer_i = interface_ptr<read_writer>;

class closer {
public:
	virtual ~closer() = default;

	virtual void close() = 0;
};

using closer_i = interface_ptr<closer>;

class read_closer : public virtual reader, public virtual closer {};

using read_closer_i = interface_ptr<read_closer>;

class write_closer : public virtual writer, public virtual closer {};

using write_closer_i = interface_ptr<write_closer>;

class read_write_closer : public virtual read_writer, public virtual closer {};

using read_write_closer_i = interface_ptr<read_write_closer>;

class seeker {
public:
	virtual ~seeker() = default;

	virtual size_t seek(size_t offset, int whence) = 0;
};

using seeker_i = interface_ptr<seeker>;

class read_seeker : public virtual reader, public virtual seeker {};

using read_seeker_i = interface_ptr<read_seeker>;

class write_seeker : public virtual writer, public virtual seeker {};

using write_seeker_i = interface_ptr<write_seeker>;

class read_write_seeker : public virtual reader, public virtual writer, public virtual seeker {};

using read_write_seeker_i = interface_ptr<read_write_seeker>;

class reader_at {
public:
	virtual ~reader_at() = default;

	virtual size_t read_at(ptrdiff_t pos, bin_ref) = 0;
};

using reader_at_i = interface_ptr<reader_at>;

class writer_at {
public:
	virtual ~writer_at() = default;

	virtual size_t write_at(ptrdiff_t pos, bin_view) = 0;
};

using writer_at_i = interface_ptr<writer_at>;

class read_writer_at : public virtual reader_at, public virtual writer_at {};

using read_writer_at_i = interface_ptr<read_writer_at>;

}
}

#endif
