#ifndef _RUA_BYTES_HPP
#define _RUA_BYTES_HPP

#include "mem/get.hpp"

#include "macros.hpp"
#include "any_ptr.hpp"
#include "nullpos.hpp"

#include <cstdint>
#include <vector>
#include <cassert>

namespace rua {
	namespace bytes {
		class pattern {
			public:
				pattern(std::initializer_list<uint16_t> container) : _sz(container.size()) {
					for (auto val : container) {
						if (val < 256) {
							if (_blocks.empty() || _blocks.back().is_void || _blocks.back().size == sizeof(uintptr_t)) {
								_blocks.push_back({false, 0, 0});
							}
							mem::get<uint8_t>(&_blocks.back().value, _blocks.back().size++) = static_cast<uint8_t>(val);
						} else {
							if (_blocks.empty() || !_blocks.back().is_void) {
								_blocks.push_back({true, 0, 0});
							}
							++_blocks.back().size;
						}
					}
				}

				size_t size() const {
					return _sz;
				}

				struct block {
					bool is_void;
					size_t size;
					uintptr_t value;

					template <typename Data>
					bool compare(const Data &dat, size_t pos = 0) const {
						if (is_void) {
							return true;
						}
						switch (size) {
							case 8:
								return mem::get<uint64_t>(&value) == dat.template get<uint64_t>(pos);

							////////////////////////////////////////////////////////////

							case 7:
								if (mem::get<uint8_t>(&value, 6) != dat.template get<uint8_t>(pos + 6)) {
									return false;
								}
								RUA_FALLTHROUGH;

							case 6:
								if (
									mem::get<uint32_t>(&value) == dat.template get<uint32_t>(pos) &&
									mem::get<uint16_t>(&value, 4) == dat.template get<uint16_t>(pos + 4)
								) {
									return true;
								}
								return false;

							////////////////////////////////////////////////////////////

							case 5:
								if (mem::get<uint8_t>(&value, 4) != dat.template get<uint8_t>(pos + 4)) {
									return false;
								}
								RUA_FALLTHROUGH;

							case 4:
								return mem::get<uint32_t>(&value) == dat.template get<uint32_t>(pos);

							////////////////////////////////////////////////////////////

							case 3:
								if (mem::get<uint8_t>(&value, 2) != dat.template get<uint8_t>(pos + 2)) {
									return false;
								}
								RUA_FALLTHROUGH;

							case 2:
								return mem::get<uint16_t>(&value) == dat.template get<uint16_t>(pos);

							////////////////////////////////////////////////////////////

							case 1:
								return mem::get<uint8_t>(&value) == dat.template get<uint8_t>(pos);

							////////////////////////////////////////////////////////////

							default:
								for (size_t i = 0; i < size; ++i) {
									if (mem::get<uint8_t>(&value, i) != dat.template get<uint8_t>(pos + i)) {
										return false;
									}
								}
						}
						return true;
					}
				};

				const std::vector<block> &blocks() const {
					return _blocks;
				}

				std::vector<size_t> void_block_poss(size_t base = 0) const {
					std::vector<size_t> vbposs(_void_block_poss);
					if (base) {
						for (auto &vbpos : vbposs) {
							vbpos += base;
						}
					}
					return vbposs;
				}

			private:
				size_t _sz;
				std::vector<block> _blocks;
				std::vector<size_t> _void_block_poss;
		};

		template <typename Data>
		class operation {
			public:
				template <typename RelPtr>
				any_ptr derel(ptrdiff_t pos = 0) const {
					return _this()->base() + pos + _this()->template get<RelPtr>(pos) + sizeof(RelPtr);
				}

				struct match_result_t {
					size_t pos;
					std::vector<size_t> void_block_poss;

					operator bool() const {
						return pos != nullpos;
					}

					size_t operator[](size_t ix) const {
						return void_block_poss[ix];
					}
				};

				match_result_t match(const pattern &pat) const {
					if (!pat.size()) {
						return match_result_t{ nullpos, {} };
					}
					size_t end = _this()->size() ? _this()->size() + 1 - pat.size() : 0;
					for (size_t i = 0; i < end || !end; ++i) {
						size_t j = 0;
						for (auto &blk : pat.blocks()) {
							if (blk.compare(*_this(), i + j)) {
								j += blk.size;
							} else {
								break;
							}
						}
						if (j == pat.size()) {
							return match_result_t{ i, pat.void_block_poss(i) };
						}
					}
					return match_result_t{ nullpos, {} };
				}

			private:
				Data *_this() {
					return static_cast<Data *>(this);
				}

				const Data *_this() const {
					return static_cast<const Data *>(this);
				}

			protected:
				operation() = default;
		};

		template<typename T>
		typename std::decay<T>::type reverse(T &&value) {
			typename std::decay<T>::type r;
			auto p = reinterpret_cast<uint8_t *>(&std::forward<T>(value));
			auto rp = reinterpret_cast<uint8_t *>(&r);
			for (size_t i = 0; i < sizeof(T); ++i) {
				rp[i] = p[sizeof(T) - 1 - i];
			}
			return r;
		}
	}
}

#endif
