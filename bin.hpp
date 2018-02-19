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

			void reset(size_t sz) {
				_sz = sz;
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

			bin_ref match(const std::vector<uint16_t> &pattern, int ret_sub_ix = -1) const {
				auto end = _sz + 1 - pattern.size();
				for (size_t i = 0; i < end; ++i) {
					size_t j;
					struct sub_range_t {
						size_t pos, sz;
					};
					std::vector<sub_range_t> subs;
					for (j = 0; j < pattern.size(); ++j) {
						if (pattern[j] > 255) {
							if (subs.empty() || subs.back().sz) {
								subs.emplace_back(sub_range_t{j, 0});
							}
							continue;
						}
						if (pattern[j] != get<uint8_t>(i + j)) {
							break;
						}
						if (subs.size() && !subs.back().sz) {
							subs.back().sz = j - subs.back().pos;
						}
					}
					if (j == pattern.size()) {
						if (ret_sub_ix > -1) {
							if (ret_sub_ix > static_cast<int>(subs.size()) - 1) {
								return nullptr;
							}
							return bin_ref(
								_data + i + subs[ret_sub_ix].pos,
								subs[ret_sub_ix].sz ? subs[ret_sub_ix].sz : j - subs[ret_sub_ix].pos,
								_rw
							);
							return nullptr;
						}
						return bin_ref(_data + i, pattern.size(), _rw);
					}
				}
				return nullptr;
			}

			unsafe_ptr match_ptr(const std::vector<uint16_t> &pattern, int ret_sub_ix = 0, uint8_t size = 0) const {
				auto md = match(pattern, ret_sub_ix);
				if (!md) {
					return nullptr;
				}
				switch (size ? (size > 8 ? 8 : size) : (md.size() > 8 ? 8 : md.size())) {
					case 8:
						return md.get<int64_t>();
					case 7:
					case 6:
					case 5:
					case 4:
						return md.get<int32_t>();
					case 3:
					case 2:
						return md.get<int16_t>();
					case 1:
						return md.get<int8_t>();
				}
				return nullptr;
			}

			unsafe_ptr match_rel_ptr(const std::vector<uint16_t> &pattern, int ret_sub_ix = 0, uint8_t size = 0) const {
				auto md = match(pattern, ret_sub_ix);
				if (!md) {
					return nullptr;
				}
				switch (size ? (size > 8 ? 8 : size) : (md.size() > 8 ? 8 : md.size())) {
					case 8:
						return md.data() + 8 + md.get<int64_t>();
					case 7:
					case 6:
					case 5:
					case 4:
						return md.data() + 4 + md.get<int32_t>();
					case 3:
					case 2:
						return md.data() + 2 + md.get<int16_t>();
					case 1:
						return md.data() + 1 + md.get<int8_t>();
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
