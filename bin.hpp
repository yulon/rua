#ifndef _TMD_BIN_HPP
#define _TMD_BIN_HPP

#include "predef.hpp"
#include "unsafe_ptr.hpp"

#ifdef _WIN32
	#include <windows.h>
	#include <psapi.h>
#endif

#include <cstdint>
#include <string>
#include <vector>
#include <utility>
#include <cassert>

namespace tmd {
	class bin_view {
		public:
			#ifdef _WIN32
				static bin_view from_this_process() {
					MODULEINFO mi;
					GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &mi, sizeof(MODULEINFO));
					return bin_view(mi.lpBaseOfDll, mi.SizeOfImage);
				}
			#endif

			////////////////////////////////////////////////////////////////////

			constexpr bin_view() : _data(), _sz(0) {}

			TMD_CONSTEXPR_14 bin_view(unsafe_ptr data, size_t size) : _data(data), _sz(size) {}

			constexpr bin_view(std::nullptr_t) : bin_view() {}

			operator bool() {
				return _data;
			}

			unsafe_ptr data() const {
				return _data;
			}

			size_t size() const {
				return _sz;
			}

			template <typename D>
			D &to() {
				assert(sizeof(D) <= _sz);
				return _data.get<D>();
			}

			template <typename D>
			const D &to() const {
				assert(sizeof(D) <= _sz);
				return _data.get<D>();
			}

			template <typename D>
			D &get(size_t pos) {
				assert(pos + sizeof(D) <= _sz);
				return _data.get<D>(pos);
			}

			template <typename D>
			const D &get(size_t pos) const {
				assert(pos + sizeof(D) <= _sz);
				return _data.get<D>(pos);
			}

			template <typename D>
			D &aligned_get(size_t ix) {
				assert((ix + 1) * sizeof(D) <= _sz);
				return _data.aligned_get<D>(ix);
			}

			template <typename D>
			const D &aligned_get(size_t ix) const {
				assert((ix + 1) * sizeof(D) <= _sz);
				return _data.aligned_get<D>(ix);
			}

			uint8_t &operator[](size_t ix) {
				assert(ix + 1 <= _sz);
				return _data[ix];
			}

			uint8_t operator[](size_t ix) const {
				assert(ix + 1 <= _sz);
				return _data[ix];
			}

			static constexpr size_t npos = -1;

			bin_view match(const std::vector<uint16_t> &pattern, bool ret_sub = false) const {
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
						if (pattern[j] != _data[i + j]) {
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
								return bin_view(_data + i + sub_j, sub_sz);
							}

							return nullptr;
						}
						return bin_view(_data + i, pattern.size());
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
						return md.to<uint64_t>();
					case 4:
						return md.to<uint32_t>();
					case 2:
						return md.to<uint16_t>();
					case 1:
						return md.to<uint8_t>();
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
						return md.data() + 8 + md.to<uint64_t>();
					case 4:
						return md.data() + 4 + md.to<uint32_t>();
					case 2:
						return md.data() + 2 + md.to<uint16_t>();
					case 1:
						return md.data() + 1 + md.to<uint8_t>();
				}
				return nullptr;
			}

		private:
			unsafe_ptr _data;
			size_t _sz;
	};
}

#endif
