#ifndef _TMD_BIN_HPP
#define _TMD_BIN_HPP

#ifdef _WIN32
	#include <windows.h>
	#include <psapi.h>
#endif

#include <vector>

namespace tmd {
	class bin {
		public:
			using ptr_t = uint8_t *;

			ptr_t base;
			size_t size;

			#ifdef _WIN32
				static bin from_this_process_main_module() {
					MODULEINFO mi;
					GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &mi, sizeof(MODULEINFO));
					return {reinterpret_cast<ptr_t>(mi.lpBaseOfDll), mi.SizeOfImage};
				}
			#endif

			uint8_t *match(const std::vector<uint16_t> &pattern, bool ret_slot_addr = false) {
				for (size_t i = 0; i < size; i++) {
					size_t j;
					int slot_off = -1;
					for (j = 0; i + j < size && j < pattern.size(); j++) {
						if (pattern[j] > 255) {
							if (slot_off == -1) {
								slot_off = j;
							}
							continue;
						}
						if (pattern[j] != base[i + j]) {
							break;
						}
					}
					if (j == pattern.size()) {
						if (ret_slot_addr) {
							return base + i + (slot_off == -1 ? j : slot_off);
						}
						return base + i;
					}
				}
				return nullptr;
			}

			ptr_t match_ptr(const std::vector<uint16_t> &pattern) {
				auto addr = match(pattern, true);
				if (!addr) {
					return nullptr;
				}
				auto pat_off = base - addr;
				auto pat_max_off = pattern.size() - 1;
				for (size_t i = 0;; ++i) {
					size_t ptr_sz;
					if (pattern[i] < 256) {
						ptr_sz = i - pat_off;
					} else if (i == pat_max_off) {
						ptr_sz = pattern.size() - pat_off;
					} else {
						continue;
					}
					switch (ptr_sz > 8 ? 8 : ptr_sz) {
						case 8:
							return reinterpret_cast<ptr_t>(static_cast<uintptr_t>(*reinterpret_cast<uint64_t *>(addr)));
						case 4:
							return reinterpret_cast<ptr_t>(static_cast<uintptr_t>(*reinterpret_cast<uint32_t *>(addr)));
						case 2:
							return reinterpret_cast<ptr_t>(static_cast<uintptr_t>(*reinterpret_cast<uint16_t *>(addr)));
						case 1:
							return reinterpret_cast<ptr_t>(static_cast<uintptr_t>(*addr));
					}
					break;
				}
				return nullptr;
			}

			ptr_t match_rel_ptr(const std::vector<uint16_t> &pattern) {
				auto addr = match(pattern, true);
				if (!addr) {
					return nullptr;
				}
				auto pat_off = base - addr;
				auto pat_max_off = pattern.size() - 1;
				for (size_t i = 0;; ++i) {
					size_t ptr_sz;
					if (pattern[i] < 256) {
						ptr_sz = i - pat_off;
					} else if (i == pat_max_off) {
						ptr_sz = pattern.size() - pat_off;
					} else {
						continue;
					}
					switch (ptr_sz > 8 ? 8 : ptr_sz) {
						case 8:
							return addr + 8 + *reinterpret_cast<uint64_t *>(addr);
						case 4:
							return addr + 4 + *reinterpret_cast<uint32_t *>(addr);
						case 2:
							return addr + 2 + *reinterpret_cast<uint16_t *>(addr);
						case 1:
							return addr + 1 + *addr;
					}
					break;
				}
				return nullptr;
			}
	};
}

#endif
