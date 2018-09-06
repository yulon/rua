#ifndef _RUA_IO_ITFS_HPP
#define _RUA_IO_ITFS_HPP

#include "../bin.hpp"
#include "../any_ptr.hpp"
#include "../limits.hpp"

#include <cstddef>

namespace rua { namespace io {
	class reader {
		public:
			virtual ~reader() = default;

			virtual size_t read(bin_ref) = 0;

			inline size_t read_full(bin_ref);

			inline bin read_all(size_t buf_sz = 1024);
	};

	class writer {
		public:
			virtual ~writer() = default;

			virtual size_t write(bin_view) = 0;

			inline bool write_all(bin_view);

			inline bool copy(reader &r, size_t buf_sz = 1024);
	};

	class read_writer : public virtual reader, public virtual writer {
		public:
			virtual ~read_writer() = default;
	};

	class seeker {
		public:
			virtual ~seeker() = default;

			virtual size_t seek(size_t offset, int whence) = 0;
	};

	class read_seeker : public virtual reader, public virtual seeker {
		public:
			virtual ~read_seeker() = default;
	};

	class write_seeker : public virtual writer, public virtual seeker {
		public:
			virtual ~write_seeker() = default;
	};

	class read_write_seeker : public virtual reader, public virtual writer, public virtual seeker {
		public:
			virtual ~read_write_seeker() = default;
	};

	class reader_at {
		public:
			virtual ~reader_at() = default;

			virtual size_t read_at(ptrdiff_t pos, bin_ref) = 0;
	};

	class writer_at {
		public:
			virtual ~writer_at() = default;

			virtual size_t write_at(ptrdiff_t pos, bin_view) = 0;
	};

	class read_writer_at : public virtual reader_at, public virtual writer_at {
		public:
			virtual ~read_writer_at() = default;
	};
}}

#endif
