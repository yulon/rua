#ifndef _RUA_IO_ITFS_HPP
#define _RUA_IO_ITFS_HPP

#include "../generic/any_ptr.hpp"

#include "../poly.hpp"

#include <cstddef>

namespace rua {
	namespace io {
		class reader_c {
			public:
				virtual ~reader_c() = default;

				virtual mem::data read(size_t size = static_cast<size_t>(-1)) const = 0;
		};

		using reader = itf<reader_c>;

		class writer_c {
			public:
				virtual ~writer_c() = default;

				virtual size_t write(const mem::data &) = 0;
		};

		using writer = itf<writer_c>;

		class read_writer_c : public virtual reader_c, public virtual writer_c {
			public:
				virtual ~read_writer_c() = default;
		};

		using read_writer = itf<read_writer_c>;

		class reader_at_c {
			public:
				virtual ~reader_at_c() = default;

				virtual mem::data read_at(ptrdiff_t pos, size_t size = static_cast<size_t>(-1)) const = 0;
		};

		using reader_at = itf<reader_at_c>;

		class writer_at_c {
			public:
				virtual ~writer_at_c() = default;

				virtual size_t write_at(ptrdiff_t pos, const mem::data &) = 0;
		};

		using writer_at = itf<writer_at_c>;

		class read_writer_at_c : public virtual reader_at_c, public virtual writer_at_c {
			public:
				virtual ~read_writer_at_c() = default;
		};

		using read_writer_at = itf<read_writer_at_c>;

		class allocator_c {
			public:
				virtual ~allocator_c() = default;

				virtual any_ptr alloc(size_t) = 0;

				virtual void free(any_ptr) = 0;
		};

		using allocator = itf<allocator_c>;
	}
}

#endif
