#ifndef _TMD_BIN_HPP
#define _TMD_BIN_HPP

#include "predef.hpp"
#include "unsafe_ptr.hpp"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <functional>
#include <utility>
#include <cassert>

namespace tmd {
	class bin_ref {
		public:
			class read_writer_t {
				public:
					using reader_t = std::function<void(unsafe_ptr src, unsafe_ptr data, size_t)>;
					using writer_t = std::function<void(unsafe_ptr dest, unsafe_ptr data, size_t)>;

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

			bin_ref() : _data(nullptr), _sz(0) {}

			bin_ref(std::nullptr_t) : bin_ref() {}

			bin_ref(unsafe_ptr data, size_t size = -1) : _data(data), _sz(size) {}

			bin_ref(unsafe_ptr data, size_t size, const read_writer_t &rw) : _data(data), _sz(size), _rw(rw) {}

			bin_ref(unsafe_ptr data, const read_writer_t &rw) : bin_ref(data, -1, rw) {}

			operator bool() const {
				return _data;
			}

			unsafe_ptr data() const {
				return _data;
			}

			size_t size() const {
				return _sz;
			}

			const read_writer_t &read_writer() const {
				return _rw;
			}

			void reset(unsafe_ptr data, size_t sz) {
				_data = data;
				_sz = sz;
			}

			void reset(unsafe_ptr data) {
				reset(data, data ? -1 : 0);
			}

			void reset(unsafe_ptr data, size_t sz, const read_writer_t &rw) {
				reset(data, data ? (sz ? sz : -1) : 0);
				_rw = rw;
			}

			void reset() {
				reset(nullptr, 0, {nullptr, nullptr});
			}

			template <typename D>
			D &at(size_t pos = 0) {
				assert(pos + sizeof(D) <= _sz && !_rw);

				return *(_data + pos).to<D *>();
			}

			template <typename D>
			const D &at(size_t pos = 0) const {
				assert(pos + sizeof(D) <= _sz && !_rw);

				return *(_data + pos).to<D *>();
			}

			template <typename D>
			D &aligned_at(size_t ix = 0) {
				assert((ix + 1) * sizeof(D) <= _sz && !_rw);

				return _data.to<D *>()[ix];
			}

			template <typename D>
			const D &aligned_at(size_t ix = 0) const {
				assert((ix + 1) * sizeof(D) <= _sz && !_rw);

				return _data.to<D *>()[ix];
			}

			uint8_t &operator[](size_t ix) {
				return aligned_at<uint8_t>(ix);
			}

			uint8_t operator[](size_t ix) const {
				return aligned_at<uint8_t>(ix);
			}

			template <typename D>
			D get(size_t pos = 0) const {
				assert(pos + sizeof(D) <= _sz);

				if (_rw.read) {
					uint8_t cache[sizeof(D)];
					_rw.read(_data + pos, &cache, sizeof(D));
					return *reinterpret_cast<D *>(cache);
				}
				return at<D>(pos);
			}

			template <typename D>
			D aligned_get(size_t ix = 0) const {
				assert((ix + 1) * sizeof(D) <= _sz);

				if (_rw.read) {
					uint8_t cache[sizeof(D)];
					_rw.read(_data + ix * sizeof(D), &cache, sizeof(D));
					return *reinterpret_cast<D *>(cache);
				}
				return aligned_at<D>(ix);
			}

			template <typename D>
			void set(size_t pos, const D &d) {
				assert(pos + sizeof(D) <= _sz);

				if (_rw.write) {
					_rw.write(_data + pos, &d, sizeof(D));
					return;
				}
				at<D>(pos) = d;
			}

			template <typename D>
			D aligned_set(size_t ix, const D &d) {
				assert((ix + 1) * sizeof(D) <= _sz);

				if (_rw.write) {
					_rw.write(_data + ix * sizeof(D), &d, sizeof(D));
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

			void copy(const bin_ref &src) {
				auto sz = size() <= src.size() ? size() : src.size();
				if (read_writer().write) {
					if (src.read_writer().read) {
						read_writer_t::copy(read_writer().write, data(), src.read_writer().read, src.data(), sz);
					} else {
						read_writer().write(data(), src.data(), sz);
					}
				} else {
					if (src.read_writer().read) {
						auto cache = new uint8_t[sz];
						src.read_writer().read(src.data(), cache, sz);
						memcpy(data(), cache, sz);
						delete[] cache;
					} else {
						memcpy(data(), src.data(), sz);
					}
				}
			}

			static constexpr size_t npos = -1;

			bin_ref match(const std::vector<uint16_t> &pattern, bool ret_sub = false) const {
				auto end = _sz + 1 - pattern.size();
				for (size_t i = 0; i < end; ++i) {
					size_t j;
					size_t sub_j = npos;
					for (j = 0; j < pattern.size(); ++j) {
						if (pattern[j] > 255) {
							if (sub_j == npos) {
								sub_j = j;
							}
							continue;
						}
						if (pattern[j] != get<uint8_t>(i + j)) {
							break;
						}
					}
					if (j == pattern.size()) {
						if (ret_sub) {
							if (sub_j == npos) {
								return nullptr;
							}

							auto k_max = pattern.size() - 1;

							for (size_t k = sub_j;; ++k) {
								size_t sub_sz;
								if (pattern[k] < 256) {
									sub_sz = k - sub_j;
								} else if (k == k_max) {
									sub_sz = pattern.size() - sub_j;
								} else {
									continue;
								}
								return bin_ref(_data + i + sub_j, sub_sz, _rw);
							}

							return nullptr;
						}
						return bin_ref(_data + i, pattern.size(), _rw);
					}
				}
				return nullptr;
			}

			unsafe_ptr match_ptr(const std::vector<uint16_t> &pattern) const {
				auto md = match(pattern, true);
				if (!md) {
					return nullptr;
				}
				switch (md.size() > 8 ? 8 : md.size()) {
					case 8:
						return md.get<uint64_t>();
					case 4:
						return md.get<uint32_t>();
					case 2:
						return md.get<uint16_t>();
					case 1:
						return md.get<uint8_t>();
				}
				return nullptr;
			}

			unsafe_ptr match_rel_ptr(const std::vector<uint16_t> &pattern) const {
				auto md = match(pattern, true);
				if (!md) {
					return nullptr;
				}
				switch (md.size() > 8 ? 8 : md.size()) {
					case 8:
						return md.data() + 8 + md.get<uint64_t>();
					case 4:
						return md.data() + 4 + md.get<uint32_t>();
					case 2:
						return md.data() + 2 + md.get<uint16_t>();
					case 1:
						return md.data() + 1 + md.get<uint8_t>();
				}
				return nullptr;
			}

		private:
			unsafe_ptr _data;
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

			bin(unsafe_ptr src_data, size_t size) : bin_ref(new uint8_t[size], size) {
				memcpy(data(), src_data, size);
			}

			bin(
				size_t size,
				const allocator_t &alcr = {nullptr, nullptr},
				const read_writer_t &rw = {nullptr, nullptr}
			) : bin_ref(alcr.alloc ? alcr.alloc(size) : unsafe_ptr(new uint8_t[size]), size, rw), _alcr(alcr) {}

			bin(
				unsafe_ptr src_data,
				size_t size,
				const allocator_t &alcr = {nullptr, nullptr},
				const read_writer_t &rw = {nullptr, nullptr}
			) : bin(size, alcr, rw) {
				if (rw.write) {
					rw.write(data(), src_data, size);
				} else {
					memcpy(data(), src_data, size);
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

			void reset() {
				if (*this) {
					if (_alcr.free) {
						_alcr.free(data(), size());
					} else {
						delete data().to<uint8_t *>();
					}
					bin_ref::reset();
				}
			}

			void reset(size_t sz) {
				if (*this) {
					if (sz == size()) {
						return;
					}
					if (_alcr.free) {
						_alcr.free(data(), size());
					} else {
						delete data().to<uint8_t *>();
					}
					if (!sz) {
						bin_ref::reset(nullptr, 0);
						return;
					}
				}
				bin_ref::reset(_alcr.alloc ? _alcr.alloc(sz) : unsafe_ptr(new uint8_t[sz]), sz);
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

			bin(bin &&src) : bin_ref(src.data(), src.size(), src.read_writer()), _alcr(src._alcr) {
				if (src) {
					static_cast<bin_ref &>(src).reset();
				}
			}

			bin &operator=(bin &&src) {
				reset();
				if (!src) {
					return *this;
				}
				bin_ref::reset(src.data(), src.size(), src.read_writer());
				_alcr = src._alcr;
				static_cast<bin_ref &>(src).reset();
				return *this;
			}

		private:
			allocator_t _alcr;
	};
}

#endif
