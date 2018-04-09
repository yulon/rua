#ifndef _RUA_BYTES_HPP
#define _RUA_BYTES_HPP

#include "generic/any_ptr.hpp"
#include "mem/get.hpp"

#include "macros.hpp"

#include <cstdint>
#include <vector>
#include <cassert>

#include <iostream>

namespace rua {
	namespace bytes {
		class pattern {
			public:
				pattern(std::initializer_list<uint16_t> container) : _sz(container.size()), _sz_rmdr(container.size() % sizeof(uintptr_t)) {
					if (!_sz) {
						return;
					}

					_words.emplace_back(word{0, 0});

					bool in_void = false;
					size_t last_sz = 0;

					for (size_t i = 0; i < _sz; ++i) {
						if (container.begin()[i] < 256) {
							if (in_void) {
								in_void = false;
							}
							if (last_sz == sizeof(uintptr_t)) {
								last_sz = 0;
								_words.emplace_back(word{0, 0});
							}
							mem::get<uint8_t>(&_words.back().mask, last_sz) = 255;
							mem::get<uint8_t>(&_words.back().value, last_sz) = static_cast<uint8_t>(container.begin()[i]);
						} else {
							if (!in_void) {
								in_void = true;
								_void_block_poss.emplace_back(i);
							}
							if (last_sz == sizeof(uintptr_t)) {
								if (_words.back().is_void()) {
									if (_words.back().value) {
										++_words.back().value;
									} else {
										_words.back().value = sizeof(uintptr_t) + 1;
									}
									++last_sz;
									continue;
								}
								last_sz = 0;
								_words.emplace_back(word{0, 0});
							}
							mem::get<uint8_t>(&_words.back().mask, last_sz) = 0;
							mem::get<uint8_t>(&_words.back().value, last_sz) = 0;
						}
						++last_sz;
					}
				}

				size_t size() const {
					return _sz;
				}

				struct word {
					uintptr_t mask, value;

					bool is_void() const {
						return !mask;
					}

					template <bool IsVoid>
					inline size_t size() const;

					inline size_t size() const;

					inline size_t compare(any_ptr ptr) const;

					inline size_t compare(any_ptr ptr, size_t sz) const;
				};

				const std::vector<word> &words() const {
					return _words;
				}

				size_t size_remainder() const {
					return _sz_rmdr;
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
				size_t _sz_rmdr;
				std::vector<word> _words;
				std::vector<size_t> _void_block_poss;
		};

		template <>
		inline size_t pattern::word::size<true>() const {
			return value;
		}

		template <>
		inline size_t pattern::word::size<false>() const {
			return sizeof(uintptr_t);
		}

		inline size_t pattern::word::size() const {
			return is_void() ? size<true>() : size<false>();
		}

		inline size_t pattern::word::compare(any_ptr ptr) const {
			if (is_void()) {
				return size<true>();
			}
			return value == (mem::get<uintptr_t>(ptr) & mask) ? size<false>() : 0;
		}

		inline size_t pattern::word::compare(any_ptr ptr, size_t sz) const {
			if (is_void()) {
				return sz;
			}

			switch (sz) {
				case 7:
					if (
						mem::get<uint8_t>(&value, 6) !=
						(
							mem::get<uint8_t>(ptr, 6) &
							mem::get<uint8_t>(&mask, 6)
						)
					) {
						return 0;
					}
					RUA_FALLTHROUGH;

				case 6:
					if (
						mem::get<uint32_t>(&value) ==
						(
							mem::get<uint32_t>(ptr) &
							mem::get<uint32_t>(&mask)
						) &&
						mem::get<uint16_t>(&value, 4) ==
						(
							mem::get<uint16_t>(ptr, 4) &
							mem::get<uint16_t>(&mask, 4)
						)
					) {
						return sz;
					}
					return 0;

				////////////////////////////////////////////////////////////

				case 5:
					if (
						mem::get<uint8_t>(&value, 4) !=
						(
							mem::get<uint8_t>(ptr, 4) &
							mem::get<uint8_t>(&mask, 4)
						)
					) {
						return 0;
					}
					RUA_FALLTHROUGH;

				case 4:
					return
						mem::get<uint32_t>(&value) ==
						(
							mem::get<uint32_t>(ptr) &
							mem::get<uint32_t>(&mask)
						)
					;

				////////////////////////////////////////////////////////////

				case 3:
					if (
						mem::get<uint8_t>(&value, 2) !=
						(
							mem::get<uint8_t>(ptr, 2) &
							mem::get<uint8_t>(&mask, 2)
						)
					) {
						return 0;
					}
					RUA_FALLTHROUGH;

				case 2:
					return
						mem::get<uint16_t>(&value) ==
						(
							mem::get<uint16_t>(ptr) &
							mem::get<uint16_t>(&mask)
						)
					;

				////////////////////////////////////////////////////////////

				case 1:
					return
						mem::get<uint8_t>(&value) ==
						(
							mem::get<uint8_t>(ptr) &
							mem::get<uint8_t>(&mask)
						)
					;

				////////////////////////////////////////////////////////////

				default:
					for (size_t i = 0; i < sz; ++i) {
						if (
							mem::get<uint8_t>(&value, i) !=
							(
								mem::get<uint8_t>(ptr, i) &
								mem::get<uint8_t>(&mask, i)
							)
						) {
							return 0;
						}
					}
			}

			return sz;
		}

		template <typename Data>
		class operation {
			public:
				template <typename RelPtr>
				any_ptr derel(ptrdiff_t pos = 0) const {
					return _this()->base() + pos + _this()->template get<RelPtr>(pos) + sizeof(RelPtr);
				}

				struct match_result_t {
					static constexpr size_t npos = static_cast<size_t>(-1);

					size_t pos;
					std::vector<size_t> void_block_poss;

					operator bool() const {
						return pos != npos;
					}

					size_t operator[](size_t ix) const {
						assert(void_block_poss.size());

						return void_block_poss[ix];
					}
				};

				match_result_t match(const pattern &pat) const {
					if (!pat.size()) {
						return match_result_t{ match_result_t::npos, {} };
					}

					size_t end = _this()->size() ? _this()->size() + 1 - pat.size() : 0;

					size_t sm_sz = 0;

					if (pat.size_remainder()) {
						if (pat.words().size() > 1) {
							auto ful_wd_c = pat.words().size() - 1;
							for (size_t i = 0; i < end || !end; ++i) {
								for (size_t j = 0; j < ful_wd_c; ++j) {
									auto sz = pat.words()[j].compare(_this()->base() + i + sm_sz);
									if (!sz) {
										sm_sz = 0;
										break;
									}
									sm_sz += sz;
								}
								if (sm_sz) {
									if (pat.words().back().compare(_this()->base() + i + sm_sz, pat.size_remainder())) {
										return match_result_t{ i, pat.void_block_poss(i) };
									}
									sm_sz = 0;
								}
							}
						} else {
							for (size_t i = 0; i < end || !end; ++i) {
								if (pat.words().back().compare(_this()->base() + i + sm_sz, pat.size_remainder())) {
									return match_result_t{ i, pat.void_block_poss(i) };
								}
							}
						}
					} else {
						for (size_t i = 0; i < end || !end; ++i) {
							for (auto &wd : pat.words()) {
								auto sz = wd.compare(_this()->base() + i + sm_sz);
								if (!sz) {
									sm_sz = 0;
									break;
								}
								sm_sz += sz;
							}
							if (sm_sz) {
								return match_result_t{ i, pat.void_block_poss(i) };
							}
						}
					}

					return match_result_t{ match_result_t::npos, {} };
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
