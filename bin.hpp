#ifndef _RUA_BIN_HPP
#define _RUA_BIN_HPP

#include "predef.hpp"
#include "unsafe_ptr.hpp"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <functional>
#include <utility>
#include <cassert>

namespace rua {
	class bin_ref {
		public:
			class read_writer_t {
				public:
					using reader_t = std::function<void(unsafe_ptr src, unsafe_ptr base, size_t)>;
					using writer_t = std::function<void(unsafe_ptr dest, unsafe_ptr base, size_t)>;

					reader_t read;
					writer_t write;

					operator bool() const {
						return read && write;
					}

					void copy(unsafe_ptr dest, unsafe_ptr src, size_t size) {
						assert(*this);

						auto cache = new uint8_t[size];
						read(src, cache, size);
						write(dest, cache, size);
						delete[] cache;
					}

					static void copy(const writer_t &wr, unsafe_ptr dest, const reader_t &rr, unsafe_ptr src, size_t size) {
						auto cache = new uint8_t[size];
						rr(src, cache, size);
						wr(dest, cache, size);
						delete[] cache;
					}
			};

			bin_ref() : _base(nullptr), _sz(0) {}

			bin_ref(std::nullptr_t) : bin_ref() {}

			bin_ref(unsafe_ptr base, size_t size = -1) : _base(base), _sz(size) {}

			bin_ref(unsafe_ptr base, size_t size, const read_writer_t &rw) : _base(base), _sz(size), _rw(rw) {}

			bin_ref(unsafe_ptr base, const read_writer_t &rw) : bin_ref(base, -1, rw) {}

			operator bool() const {
				return _base;
			}

			unsafe_ptr base() const {
				return _base;
			}

			void rebase(unsafe_ptr base) {
				_base = base;
			}

			size_t size() const {
				return _sz;
			}

			void resize(size_t sz) {
				_sz = sz;
			}

			const read_writer_t &read_writer() const {
				return _rw;
			}

			void reset(unsafe_ptr base, size_t sz = -1) {
				_base = base;
				_sz = sz;
			}

			void reset(unsafe_ptr base, size_t sz, const read_writer_t &rw) {
				_base = base;
				_sz = sz;
				_rw = rw;
			}

			void reset() {
				reset(nullptr, 0, {nullptr, nullptr});
			}

			template <typename D>
			D &at(ptrdiff_t pos = 0) {
				assert(pos + sizeof(D) <= _sz && !_rw);

				return *unsafe_ptr(_base.value() + pos).to<D *>();
			}

			template <typename D>
			const D &at(ptrdiff_t pos = 0) const {
				assert(pos + sizeof(D) <= _sz && !_rw);

				return *unsafe_ptr(_base.value() + pos).to<D *>();
			}

			template <typename D>
			D &aligned_at(ptrdiff_t ix = 0) {
				assert((ix + 1) * sizeof(D) <= _sz && !_rw);

				return _base.to<D *>()[ix];
			}

			template <typename D>
			const D &aligned_at(ptrdiff_t ix = 0) const {
				assert((ix + 1) * sizeof(D) <= _sz && !_rw);

				return _base.to<D *>()[ix];
			}

			uint8_t &operator[](ptrdiff_t ix) {
				return aligned_at<uint8_t>(ix);
			}

			uint8_t operator[](ptrdiff_t ix) const {
				return aligned_at<uint8_t>(ix);
			}

			template <typename D>
			D get(ptrdiff_t pos = 0) const {
				assert(pos + sizeof(D) <= _sz);

				if (_rw.read) {
					uint8_t cache[sizeof(D)];
					_rw.read(_base.value() + pos, &cache, sizeof(D));
					return *reinterpret_cast<D *>(cache);
				}
				return at<D>(pos);
			}

			template <typename D>
			D aligned_get(ptrdiff_t ix = 0) const {
				assert((ix + 1) * sizeof(D) <= _sz);

				if (_rw.read) {
					uint8_t cache[sizeof(D)];
					_rw.read(_base.value() + ix * sizeof(D), &cache, sizeof(D));
					return *reinterpret_cast<D *>(cache);
				}
				return aligned_at<D>(ix);
			}

			template <typename D>
			void set(ptrdiff_t pos, const D &d) {
				assert(pos + sizeof(D) <= _sz);

				if (_rw.write) {
					_rw.write(_base.value() + pos, &d, sizeof(D));
					return;
				}
				at<D>(pos) = d;
			}

			template <typename D>
			D aligned_set(ptrdiff_t ix, const D &d) {
				assert((ix + 1) * sizeof(D) <= _sz);

				if (_rw.write) {
					_rw.write(_base.value() + ix * sizeof(D), &d, sizeof(D));
					return;
				}
				aligned_at<D>(ix) = d;
			}

			template <typename D>
			void set(const D &d) {
				set<D>(0, d);
			}

			template <typename D>
			D aligned_set(const D &d) {
				aligned_set<D>(0, d);
			}

			template <typename RELPTR_T>
			unsafe_ptr derelative(ptrdiff_t pos = 0) const {
				return base().value() + pos + get<RELPTR_T>(pos) + sizeof(RELPTR_T);
			}

			void copy(const bin_ref &src) {
				auto sz = size() <= src.size() ? size() : src.size();
				if (read_writer().write) {
					if (src.read_writer().read) {
						read_writer_t::copy(read_writer().write, _base, src.read_writer().read, src._base, sz);
					} else {
						read_writer().write(_base, src._base, sz);
					}
				} else {
					if (src.read_writer().read) {
						auto cache = new uint8_t[sz];
						src.read_writer().read(src._base, cache, sz);
						memcpy(_base, cache, sz);
						delete[] cache;
					} else {
						memcpy(_base, src._base, sz);
					}
				}
			}

			bin_ref match(const std::vector<uint16_t> &pattern) const {
				auto end = _sz + 1 - pattern.size();
				for (size_t i = 0; i < end; ++i) {
					size_t j;
					for (j = 0; j < pattern.size(); ++j) {
						if (pattern[j] > 255) {
							continue;
						}
						if (pattern[j] != get<uint8_t>(i + j)) {
							break;
						}
					}
					if (j == pattern.size()) {
						return bin_ref(_base.value() + i, pattern.size(), _rw);
					}
				}
				return nullptr;
			}

			std::vector<bin_ref> match_sub(const std::vector<uint16_t> &pattern) const {
				std::vector<bin_ref> subs;
				auto end = _sz + 1 - pattern.size();
				for (size_t i = 0; i < end; ++i) {
					size_t j;
					for (j = 0; j < pattern.size(); ++j) {
						if (pattern[j] > 255) {
							if (subs.empty() || subs.back().size()) {
								subs.emplace_back(_base.value() + i + j, 0, _rw);
							}
							continue;
						}
						if (pattern[j] != get<uint8_t>(i + j)) {
							break;
						}
						if (subs.size() && !subs.back().size()) {
							subs.back().resize(_base.value() + i + j - subs.back().base().value());
						}
					}
					if (j == pattern.size()) {
						if (subs.size() && !subs.back().size()) {
							subs.back().resize(_base.value() + i + j - subs.back().base().value());
						}
						return subs;
					}
					subs.clear();
				}
				return subs;
			}

		private:
			unsafe_ptr _base;
			size_t _sz;
			read_writer_t _rw;
	};

	class bin : public bin_ref {
		public:
			class allocator_t {
				public:
					std::function<unsafe_ptr(size_t)> alloc;
					std::function<void(unsafe_ptr, size_t)> free;

					operator bool() const {
						return this->alloc && this->free;
					}
			};

			bin() : bin_ref() {}

			bin(std::nullptr_t) : bin() {}

			bin(size_t size) : bin_ref(new uint8_t[size], size) {}

			bin(unsafe_ptr src_base, size_t size) : bin_ref(new uint8_t[size], size) {
				memcpy(base(), src_base, size);
			}

			bin(
				size_t size,
				const allocator_t &alcr = {nullptr, nullptr},
				const read_writer_t &rw = {nullptr, nullptr}
			) : bin_ref(alcr.alloc ? alcr.alloc(size) : unsafe_ptr(new uint8_t[size]), size, rw), _alcr(alcr) {}

			bin(
				unsafe_ptr src_base,
				size_t size,
				const allocator_t &alcr = {nullptr, nullptr},
				const read_writer_t &rw = {nullptr, nullptr}
			) : bin(size, alcr, rw) {
				if (rw.write) {
					rw.write(base(), src_base, size);
				} else {
					memcpy(base(), src_base, size);
				}
			}

			bin(
				const bin_ref &src,
				const allocator_t &alcr = {nullptr, nullptr},
				const read_writer_t &rw = {nullptr, nullptr}
			) : bin(src.size(), alcr, rw) {
				copy(src);
			}

			bin(
				const bin_ref &src,
				size_t size,
				const allocator_t &alcr = {nullptr, nullptr},
				const read_writer_t &rw = {nullptr, nullptr}
			) : bin(size, alcr, rw) {
				copy(src);
			}

			allocator_t allocator() const {
				return _alcr;
			}

			void rebase(size_t new_sz = 0) {
				if (*this) {
					if (!new_sz) {
						new_sz = size();
					}
					*this = bin(*this, new_sz, _alcr, read_writer());
				}
			}

			void resize(size_t sz) {
				if (*this) {
					if (sz == size()) {
						return;
					}
					if (_alcr.free) {
						_alcr.free(base(), size());
					} else {
						delete base().to<uint8_t *>();
					}
					if (!sz) {
						bin_ref::reset(nullptr, 0);
						return;
					}
				}
				bin_ref::reset(_alcr.alloc ? _alcr.alloc(sz) : unsafe_ptr(new uint8_t[sz]), sz);
			}

			void reset(unsafe_ptr base, size_t sz) = delete;

			void reset(unsafe_ptr base, size_t sz, const read_writer_t &rw) = delete;

			void reset() {
				if (*this) {
					if (_alcr.free) {
						_alcr.free(base(), size());
					} else {
						delete base().to<uint8_t *>();
					}
					bin_ref::reset();
				}
			}

			void reset(
				size_t sz,
				const allocator_t &alcr,
				const read_writer_t &rw
			) {
				reset();
				_alcr = alcr;
				bin_ref::reset(alcr.alloc ? alcr.alloc(sz) : unsafe_ptr(new uint8_t[sz]), sz, rw);
			}

			void reset(size_t sz) {
				reset(sz, _alcr, read_writer());
			}

			void reset(
				size_t sz,
				const allocator_t &alcr
			) {
				reset(sz, alcr, read_writer());
			}

			void reset(
				size_t sz,
				const read_writer_t &rw
			) {
				reset(sz, _alcr, rw);
			}

			~bin() {
				reset();
			}

			bin(const bin &src) : bin(src.size(), src.allocator(), src.read_writer()) {
				copy(src);
			}

			bin &operator=(const bin &src) {
				if (!src) {
					reset();
					return *this;
				}
				reset(src.size(), src.allocator(), src.read_writer());
				copy(src);
				return *this;
			}

			bin(bin &&src) : bin_ref(src.base(), src.size(), src.read_writer()), _alcr(src._alcr) {
				if (src) {
					static_cast<bin_ref &>(src).reset();
				}
			}

			bin &operator=(bin &&src) {
				reset();
				if (!src) {
					return *this;
				}
				bin_ref::reset(src.base(), src.size(), src.read_writer());
				_alcr = src._alcr;
				static_cast<bin_ref &>(src).reset();
				return *this;
			}

		private:
			allocator_t _alcr;
	};
}

#endif
