#ifndef _TMD_BIN_HPP
#define _TMD_BIN_HPP

#include "unsafe_ptr.hpp"

#ifdef _WIN32
	#include <windows.h>
	#include <psapi.h>
#endif

#include <cstdint>
#include <string>
#include <vector>
#include <utility>

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

			constexpr bin_view(unsafe_ptr data, size_t size) : _data(data), _sz(size) {}

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

			static constexpr size_t npos = -1;

			bin_view match(const std::vector<uint16_t> &pattern, bool ret_sub = false) const {
				size_t sub_i = npos;
				auto end = _sz - pattern.size() + 1;
				for (size_t i = 0; i < end; i++) {
					size_t j;
					for (j = 0; i + j < _sz && j < pattern.size(); j++) {
						if (pattern[j] > 255) {
							if (sub_i == npos) {
								sub_i = j;
							}
							continue;
						}
						if (pattern[j] != _data[i + j]) {
							break;
						}
					}
					if (j == pattern.size()) {
						if (ret_sub) {
							if (sub_i == npos) {
								return nullptr;
							}

							auto sub_j = sub_i - i;
							auto sub_sz_max = pattern.size() - sub_j;
							auto sub_j_max = sub_sz_max - 1;

							for (size_t k = 0;; ++i) {
								size_t sub_sz;
								if (pattern[k] < 256) {
									sub_sz = k - sub_j;
								} else if (k == sub_j_max) {
									sub_sz = sub_sz_max;
								} else {
									continue;
								}
								return bin_view(_data + sub_i, sub_sz);
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
						return md.data().deref<uint64_t>();
					case 4:
						return md.data().deref<uint32_t>();
					case 2:
						return md.data().deref<uint16_t>();
					case 1:
						return *md.data();
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
						return _data + 8 + md.data().deref<uint64_t>();
					case 4:
						return _data + 4 + md.data().deref<uint32_t>();
					case 2:
						return _data + 2 + md.data().deref<uint16_t>();
					case 1:
						return _data + 1 + *md.data();
				}
				return nullptr;
			}

		private:
			unsafe_ptr _data;
			size_t _sz;
	};
}

#endif
