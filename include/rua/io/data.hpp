#ifndef _RUA_IO_DATA_HPP
#define _RUA_IO_DATA_HPP

#include "itfs.hpp"

#include <cassert>

namespace rua {
	namespace io {
		class data : public rua::data, public bytes::operation<data> {
			public:
				data() : rua::data(), _alr(nullptr), _rwa(nullptr) {}

				data(std::nullptr_t) : data() {}

				explicit data(size_t size, allocator alr = nullptr, read_writer_at rwa = nullptr) :
					rua::data(rua::data::_dont_init{})
				{
					if (size) {
						_alr = alr;
						_rwa = rwa;
						_alloc(size);
					} else {
						_clear();
					}
				}

				data(any_ptr base, size_t size = static_cast<size_t>(-1), allocator alr = nullptr, read_writer_at rwa = nullptr) :
					data(base, size, nullptr, alr, rwa)
				{}

				~data() {
					free();
				}

				void free() {
					if (_alloced) {
						if (!--_alloced_use_count()) {
							if (_alr) {
								_alr->free(_alloced_base_ref());
							}
							delete[] _alloced.to<uint8_t *>();
						}
					}
					_clear(0);
				}

				data(const data &src) : data(src._sz) {
					copy(src);
				}

				data &operator=(const data &src) {
					free();
					if (src) {
						_alloc(src._sz);
						copy(src);
					}
					return *this;
				}

				data(data &&src) {
					if (src) {
						_base = src._base;
						_sz = src._sz;
						_alloced = src._alloced;
						_alr = src._alr;
						_rwa = src._rwa;
						src._clear(0);
					}
				}

				data &operator=(data &&src) {
					free();
					if (src) {
						_base = src._base;
						_sz = src._sz;
						_alloced = src._alloced;
						_alr = src._alr;
						_rwa = src._rwa;
						src._clear(0);
					}
					return *this;
				}

				size_t copy(const data &src) {
					auto cp_sz = _sz < src._sz ? _sz : src._sz;
					if (!cp_sz) {
						return 0;
					}

					if (_rwa) {
						if (src._rwa) {
							_rwa->write_at(_base.signed_value(), src._rwa->read_at(src._base.signed_value(), cp_sz));
						} else {
							_rwa->write_at(base().signed_value(), static_cast<const rua::data &>(src));
						}
					} else if (src._rwa) {
						auto cache = src._rwa->read_at(src._base.signed_value(), cp_sz);
						memcpy(_base, cache.base(), cp_sz);
					} else {
						memcpy(_base, src._base, cp_sz);
					}
					return cp_sz;
				}

				template <typename D>
				D get(ptrdiff_t offset = 0) const {
					#ifndef NDEBUG
						if (_alloced) {
							assert(_base + offset >= _alloced_base());
							assert(sizeof(D) <= _alloced_size());
						}
					#endif

					if (_rwa) {
						return _rwa->read_at((_base + offset).signed_value(), sizeof(D)).get<D>();
					} else {
						return rua::data::get<D>(offset);
					}
				}

				template <typename D>
				D aligned_get(ptrdiff_t ix = 0) const {
					#ifndef NDEBUG
						if (_alloced) {
							assert(_base + ix * sizeof(D) >= _alloced_base());
							assert(sizeof(D) <= _alloced_size());
						}
					#endif

					if (_rwa) {
						return _rwa->read_at((_base + ix * sizeof(D)).signed_value(), sizeof(D)).aligned_get<D>();
					} else {
						return rua::data::aligned_get<D>(ix);
					}
				}

				template <typename V>
				void set(ptrdiff_t offset, V &&val) {
					using VV = typename std::decay<V>::type;

					#ifndef NDEBUG
						if (_alloced) {
							assert(_base + offset >= _alloced_base());
							assert(sizeof(VV) <= _alloced_size());
						}
					#endif

					if (_rwa) {
						_rwa->write_at((_base + offset).signed_value(), rua::data(&std::forward<V>(val), sizeof(VV)));
					} else {
						rua::data::get<VV>(offset) = std::forward<V>(val);
					}
				}

				template <typename V>
				void aligned_set(ptrdiff_t ix, V &&val) {
					using VV = typename std::decay<V>::type;

					#ifndef NDEBUG
						if (_alloced) {
							assert(_base + ix * sizeof(VV) >= _alloced_base());
							assert(sizeof(VV) <= _alloced_size());
						}
					#endif

					if (_rwa) {
						_rwa->write_at((_base + ix * sizeof(V)).signed_value(), rua::data(&std::forward<V>(val), sizeof(VV)));
					} else {
						rua::data::aligned_get<VV>(ix) = std::forward<V>(val);
					}
				}

				data slice(ptrdiff_t begin, ptrdiff_t end) const {
					if (end < begin) {
						auto real_end = begin;
						begin = end;
						end = real_end;
					}

					auto new_base = _base + begin;
					auto new_sz = static_cast<size_t>(end - begin);

					if (_alloced) {
						#ifndef NDEBUG
							assert(new_base >= _alloced_base());
							assert(new_sz <= _alloced_size());
						#endif

						++_alloced_use_count();
					}

					return data(new_base, new_sz, _alloced, _alr, _rwa);
				}

				data slice(ptrdiff_t begin) const {
					auto new_base = _base + begin;
					auto new_sz = begin > 0 ? _sz - static_cast<size_t>(begin) : _sz + static_cast<size_t>(0 - begin);

					if (_alloced) {
						#ifndef NDEBUG
							assert(new_base >= _alloced_base());
							assert(new_sz <= _alloced_size());
						#endif

						++_alloced_use_count();
					}

					return data(new_base, new_sz, _alloced, _alr, _rwa);
				}

				data operator()(ptrdiff_t begin, ptrdiff_t end) const {
					return slice(begin, end);
				}

				data operator[](ptrdiff_t begin) const {
					return slice(begin);
				}

				data ref() const {
					return slice(0);
				}

				data original_ref() const {
					return _alloced ?
						data(_alloced_base(), _alloced_size(), _alloced, _alr, _rwa) :
						data(_base, _sz, nullptr, _alr, _rwa)
					;
				}

				rua::data to_local() const {
					if (_rwa) {
						return _rwa->read_at(base().signed_value(), _sz);
					} else if (_alr) {
						return *static_cast<const rua::data *>(this);
					} else {
						return static_cast<const rua::data *>(this)->ref();
					}
				}

				rua::data to_local_ref() const {
					assert(!_alr && !_rwa);

					return static_cast<const rua::data *>(this)->ref();
				}

			protected:
				allocator _alr;
				read_writer_at _rwa;

				data(any_ptr base, size_t size, any_ptr alloced, allocator alr, read_writer_at rwa) : rua::data(base, size, alloced), _alr(alr), _rwa(rwa) {}

				any_ptr &_alloced_base_ref() const {
					return *(_alloced + sizeof(std::atomic<size_t>) + sizeof(size_t)).to<any_ptr *>();
				}

				any_ptr _alloced_base() const {
					if (_alr) {
						return _alloced_base_ref();
					} else {
						return rua::data::_alloced_base();
					}
				}

				void _alloc(size_t size) {
					if (_alr) {
						_alloced = new uint8_t[sizeof(std::atomic<size_t>) + sizeof(size_t) + sizeof(any_ptr)];
						new (&_alloced_use_count()) std::atomic<size_t>(1);
						_alloced_size() = size;
						_alloced_base_ref() = _alr->alloc(size);

						_base = _alloced_base_ref();
						_sz = size;
					} else {
						rua::data::_alloc(size);
					}
				}

				void _clear(size_t sz = static_cast<size_t>(-1)) {
					rua::data::_clear(sz);
					_alr = nullptr;
					_rwa = nullptr;
				}
		};
	}
}

#endif
