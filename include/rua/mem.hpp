#ifndef _RUA_MEM_HPP
#define _RUA_MEM_HPP

#include "macros.hpp"
#include "any_ptr.hpp"
#include "bytes.hpp"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <functional>
#include <utility>
#include <atomic>
#include <cassert>

namespace rua {
	namespace mem {
		template <typename T>
		static inline T &get(any_ptr ptr, ptrdiff_t offset = 0) {
			return *(ptr + offset).to<T *>();
		}

		template <typename T>
		static inline T &aligned_get(any_ptr ptr, ptrdiff_t ix = 0) {
			return get<T>(ptr, ix * sizeof(T));
		}

		class getter {
			public:
				template <typename T>
				T &get(ptrdiff_t offset = 0) const {
					return mem::get<T>(this, offset);
				}

				template <typename T>
				T &aligned_get(ptrdiff_t ix = 0) {
					return get<T>(ix * sizeof(T));
				}
		};

		class data : public bytes::operation<data> {
			public:
				constexpr data() : _base(nullptr), _sz(0), _alloced(nullptr) {}

				constexpr data(std::nullptr_t) : data() {}

				explicit data(size_t size) {
					if (size) {
						_alloc(size);
					} else {
						_clear();
					}
				}

				data(any_ptr base, size_t size = 0) : data(base, size, nullptr) {}

				~data() {
					free();
				}

				void free() {
					if (_alloced) {
						if (!--_alloced_use_count()) {
							delete[] _alloced.to<uint8_t *>();
						}
						_alloced = nullptr;
					}
					_base = nullptr;
					_sz = 0;
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
						src._clear();
					}
				}

				data &operator=(data &&src) {
					free();
					if (src) {
						_base = src._base;
						_sz = src._sz;
						_alloced = src._alloced;
						src._clear();
					}
					return *this;
				}

				operator bool() const {
					return _base;
				}

				any_ptr base() const {
					return _base;
				}

				size_t size() const {
					return _sz;
				}

				size_t copy(const data &src) {
					auto cp_sz = _sz < src._sz ? _sz : src._sz;
					if (!cp_sz) {
						return 0;
					}

					memcpy(_base, src._base, cp_sz);
					return cp_sz;
				}

				template <typename D>
				D &get(ptrdiff_t offset = 0) const {
					#ifndef NDEBUG
						if (_alloced) {
							assert(_base + offset >= _alloced_base());
							assert(sizeof(D) <= _alloced_size());
						}
					#endif

					return mem::get<D>(_base, offset);
				}

				template <typename D>
				D &aligned_get(ptrdiff_t ix = 0) const {
					#ifndef NDEBUG
						if (_alloced) {
							assert(_base + ix * sizeof(D) >= _alloced_base());
							assert(sizeof(D) <= _alloced_size());
						}
					#endif

					return mem::aligned_get<D>(_base, ix);
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

					return data(new_base, new_sz, _alloced);
				}

				data slice(ptrdiff_t begin) const {
					auto new_base = _base + begin;
					auto new_sz = static_cast<size_t>(static_cast<ptrdiff_t>(_sz) - begin);

					if (_alloced) {
						#ifndef NDEBUG
							assert(new_base >= _alloced_base());
							assert(new_sz <= _alloced_size());
						#endif

						++_alloced_use_count();
					}

					return data(new_base, new_sz, _alloced);
				}

				data operator()(ptrdiff_t begin, ptrdiff_t end) const {
					return slice(begin, end);
				}

				data operator[](ptrdiff_t offset) const {
					return slice(offset);
				}

				data ref() const {
					return slice(0);
				}

				size_t use_count() const {
					return _alloced ? _alloced_use_count().load() : 0;
				}

				data original_ref() const {
					return _alloced ?
						data(_alloced_base(), _alloced_size(), _alloced) :
						data(_base, _sz, nullptr)
					;
				}

			protected:
				any_ptr _base;
				size_t _sz;
				any_ptr _alloced;

				struct _dont_init {};
				data(_dont_init) {}

				data(any_ptr base, size_t size, any_ptr alloced) : _base(base), _sz(size), _alloced(alloced) {}

				std::atomic<size_t> &_alloced_use_count() const {
					return *_alloced.to<std::atomic<size_t> *>();
				}

				size_t &_alloced_size() const {
					return *(_alloced + sizeof(std::atomic<size_t>)).to<size_t *>();
				}

				any_ptr _alloced_base() const {
					return _alloced + sizeof(std::atomic<size_t>) + sizeof(size_t);
				}

				void _alloc(size_t size) {
					_alloced = new uint8_t[sizeof(std::atomic<size_t>) + sizeof(size_t) + size];
					new (&_alloced_use_count()) std::atomic<size_t>(1);
					_alloced_size() = size;

					_base = _alloced_base();
					_sz = size;
				}

				void _clear() {
					_base = nullptr;
					_sz = 0;
					_alloced = nullptr;
				}
		};
	}
}

#endif
