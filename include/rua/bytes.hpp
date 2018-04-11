#ifndef _RUA_BYTES_HPP
#define _RUA_BYTES_HPP

#include "gnc/any_ptr.hpp"
#include "mem/get.hpp"

#include "macros.hpp"

#include <cstdint>
#include <vector>
#include <type_traits>
#include <cassert>

namespace rua {
	namespace bytes {
		template <typename Alignment = uintmax_t>
		class aligned {
			public:
				template <typename Container>
				aligned(const Container &byt_vals) {
					_input(byt_vals);
				}

				aligned(std::initializer_list<uint8_t> byt_vals) {
					_input(byt_vals);
				}

				size_t size() const {
					return _sz;
				}

				struct word {
					typename std::conditional<sizeof(Alignment) >= sizeof(size_t), Alignment, size_t>::type value;

					template <typename Getter>
					size_t eq(const Getter &gtr, size_t gtr_off = 0) const {
						return mem::get<Alignment>(&value) == gtr.template get<Alignment>(gtr_off) ? sizeof(Alignment) : 0;
					}

					template <typename Getter>
					size_t eq(const Getter &gtr, size_t gtr_off, size_t sz) const {
						switch (sz) {
							case 7:
								if (mem::get<uint8_t>(&value, 6) != gtr.template get<uint8_t>(gtr_off + 6)) {
									return 0;
								}
								RUA_FALLTHROUGH;

							case 6:
								if (
									mem::get<uint32_t>(&value) == gtr.template get<uint32_t>(gtr_off) &&
									mem::get<uint16_t>(&value, 4) == gtr.template get<uint16_t>(gtr_off + 4)
								) {
									return sz;
								}
								return 0;

							////////////////////////////////////////////////////////////

							case 5:
								if (mem::get<uint8_t>(&value, 4) != gtr.template get<uint8_t>(gtr_off + 4)) {
									return 0;
								}
								RUA_FALLTHROUGH;

							case 4:
								return mem::get<uint32_t>(&value) == gtr.template get<uint32_t>(gtr_off);

							////////////////////////////////////////////////////////////

							case 3:
								if (mem::get<uint8_t>(&value, 2) != gtr.template get<uint8_t>(gtr_off + 2)) {
									return 0;
								}
								RUA_FALLTHROUGH;

							case 2:
								return mem::get<uint16_t>(&value) == gtr.template get<uint16_t>(gtr_off);

							////////////////////////////////////////////////////////////

							case 1:
								return mem::get<uint8_t>(&value) == gtr.template get<uint8_t>(gtr_off);

							////////////////////////////////////////////////////////////

							default:
								for (size_t i = 0; i < sz; ++i) {
									if (mem::get<uint8_t>(&value, i) == gtr.template get<uint8_t>(gtr_off + i)) {
										return 0;
									}
								}
						}

						return sz;
					}
				};

				const std::vector<word> &words() const {
					return _words;
				}

				size_t size_remainder() const {
					return _sz_rmdr;
				}

			private:
				size_t _sz;
				size_t _sz_rmdr;
				std::vector<word> _words;

				template <typename Container>
				void _input(const Container &byt_vals) {
					_sz = byt_vals.size();

					if (!_sz) {
						return;
					}

					_sz_rmdr = byt_vals.size() % sizeof(Alignment);

					if (_sz_rmdr) {
						_words.resize(byt_vals.size() / sizeof(Alignment) + 1);
					} else {
						_words.resize(byt_vals.size() / sizeof(Alignment));
					}

					size_t wi = 0;
					size_t last_sz = 0;

					for (size_t i = 0; i < _sz; ++i) {
						if (last_sz == sizeof(Alignment)) {
							last_sz = 0;
							++wi;
						}
						mem::get<uint8_t>(&_words[wi].value, last_sz) = static_cast<uint8_t>(byt_vals.begin()[i]);
						++last_sz;
					}
				}
		};

		template <typename Alignment = uintmax_t>
		class pattern {
			public:
				template <typename Container>
				pattern(const Container &byt_vals) {
					_input(byt_vals);
				}

				pattern(std::initializer_list<uint16_t> byt_vals) {
					_input(byt_vals);
				}

				size_t size() const {
					return _sz;
				}

				struct word {
					Alignment mask;
					typename std::conditional<sizeof(Alignment) >= sizeof(size_t), Alignment, size_t>::type value;

					bool is_void() const {
						return !mask;
					}

					template <typename Getter>
					size_t eq(const Getter &gtr, size_t gtr_off = 0) const {
						if (is_void()) {
							return value;
						}
						return mem::get<Alignment>(&value) == (gtr.template get<Alignment>(gtr_off) & mask) ? sizeof(Alignment) : 0;
					}

					template <typename Getter>
					size_t eq(const Getter &gtr, size_t gtr_off, size_t sz) const {
						if (is_void()) {
							return value ? value : sz;
						}

						switch (sz) {
							case 7:
								if (
									mem::get<uint8_t>(&value, 6) !=
									(
										gtr.template get<uint8_t>(gtr_off + 6) &
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
										gtr.template get<uint32_t>(gtr_off) &
										mem::get<uint32_t>(&mask)
									) &&
									mem::get<uint16_t>(&value, 4) ==
									(
										gtr.template get<uint16_t>(gtr_off + 4) &
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
										gtr.template get<uint8_t>(gtr_off + 4) &
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
										gtr.template get<uint32_t>(gtr_off) &
										mem::get<uint32_t>(&mask)
									)
								;

							////////////////////////////////////////////////////////////

							case 3:
								if (
									mem::get<uint8_t>(&value, 2) !=
									(
										gtr.template get<uint8_t>(gtr_off + 2) &
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
										gtr.template get<uint16_t>(gtr_off) &
										mem::get<uint16_t>(&mask)
									)
								;

							////////////////////////////////////////////////////////////

							case 1:
								return
									mem::get<uint8_t>(&value) ==
									(
										gtr.template get<uint8_t>(gtr_off) &
										mem::get<uint8_t>(&mask)
									)
								;

							////////////////////////////////////////////////////////////

							default:
								for (size_t i = 0; i < sz; ++i) {
									if (
										mem::get<uint8_t>(&value, i) !=
										(
											gtr.template get<uint8_t>(gtr_off + i) &
											mem::get<uint8_t>(&mask, i)
										)
									) {
										return 0;
									}
								}
						}

						return sz;
					}
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

				template <typename Container>
				void _input(const Container &byt_vals) {
					_sz = byt_vals.size();

					if (!_sz) {
						return;
					}

					_sz_rmdr = byt_vals.size() % sizeof(Alignment);

					_words.emplace_back(word{0, 0});

					bool in_void = false;
					size_t last_sz = 0;

					for (size_t i = 0; i < _sz; ++i) {
						if (byt_vals.begin()[i] < 256) {
							if (in_void) {
								in_void = false;
							}
							if (last_sz == sizeof(Alignment)) {
								last_sz = 0;
								_words.emplace_back(word{0, 0});
							}
							mem::get<uint8_t>(&_words.back().mask, last_sz) = 255;
							mem::get<uint8_t>(&_words.back().value, last_sz) = static_cast<uint8_t>(byt_vals.begin()[i]);
						} else {
							if (!in_void) {
								in_void = true;
								_void_block_poss.emplace_back(i);
							}
							if (last_sz == sizeof(Alignment)) {
								if (_words.back().is_void()) {
									if (_words.back().value) {
										++_words.back().value;
									} else {
										_words.back().value = sizeof(Alignment) + 1;
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
		};

		template <typename Getter>
		class operation {
			public:
				template <typename RelPtr>
				any_ptr derel(ptrdiff_t pos = 0) const {
					return _this()->base() + pos + _this()->template get<RelPtr>(pos) + sizeof(RelPtr);
				}

				static constexpr size_t npos = static_cast<size_t>(-1);

				struct find_result_t {
					size_t pos;

					operator bool() const {
						return pos != npos;
					}
				};

				template <typename Alignment = uintmax_t, typename Formatted = aligned<Alignment>>
				find_result_t find(const Formatted &byts) const {
					if (!byts.size()) {
						return find_result_t{ npos };
					}

					size_t end = _this()->size() ? _this()->size() + 1 - byts.size() : 0;
					size_t sm_sz = 0;

					if (byts.size_remainder()) {
						if (byts.words().size() > 1) {
							auto ful_wd_c = byts.words().size() - 1;
							for (size_t i = 0; i < end || !end; ++i) {
								for (size_t j = 0; j < ful_wd_c; ++j) {
									auto sz = byts.words()[j].eq(*_this(), i + sm_sz);
									if (!sz) {
										sm_sz = 0;
										break;
									}
									sm_sz += sz;
								}
								if (sm_sz) {
									if (byts.words().back().eq(*_this(), i + sm_sz, byts.size_remainder())) {
										return find_result_t{ i };
									}
									sm_sz = 0;
								}
							}
						} else {
							for (size_t i = 0; i < end || !end; ++i) {
								if (byts.words().back().eq(*_this(), i + sm_sz, byts.size_remainder())) {
									return find_result_t{ i };
								}
							}
						}
					} else {
						for (size_t i = 0; i < end || !end; ++i) {
							for (auto &wd : byts.words()) {
								auto sz = wd.eq(*_this(), i + sm_sz);
								if (!sz) {
									sm_sz = 0;
									break;
								}
								sm_sz += sz;
							}
							if (sm_sz) {
								return find_result_t{ i };
							}
						}
					}

					return find_result_t{ npos };
				}

				struct match_result_t {
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

				template <typename Alignment = uintmax_t>
				match_result_t match(const pattern<Alignment> &pat) const {
					auto fr = find<Alignment, pattern<Alignment>>(pat);
					if (!fr) {
						return match_result_t{ npos, {} };
					}
					return match_result_t{ fr.pos, pat.void_block_poss(fr.pos) };
				}

			private:
				Getter *_this() {
					return static_cast<Getter *>(this);
				}

				const Getter *_this() const {
					return static_cast<const Getter *>(this);
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
