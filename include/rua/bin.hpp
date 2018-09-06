#ifndef _RUA_BIN_HPP
#define _RUA_BIN_HPP

#include "mem/get.hpp"

#include "macros.hpp"
#include "limits.hpp"

#if RUA_CPP >= RUA_CPP_17 && RUA_HAS_INC(<string_view>)
	#include <string_view>
#endif

#include <string>
#include <cstdint>
#include <utility>
#include <atomic>
#include <cstring>
#include <utility>
#include <vector>
#include <cassert>

namespace rua {
	template <typename Getter>
	class bin_base {
		public:
			template <typename RelPtr, size_t SlotSize = sizeof(RelPtr)>
			any_ptr derel(ptrdiff_t pos = 0) const {
				return _this()->base() + pos + SlotSize + _this()->template get<RelPtr>(pos);
			}

			template <typename RelPtr, size_t SlotSize = sizeof(RelPtr)>
			RelPtr enrel(ptrdiff_t pos, any_ptr abs_ptr) {
				auto rel_ptr = static_cast<RelPtr>(abs_ptr - (_this()->base() + pos + SlotSize));
				_this()->template set<RelPtr>(pos, rel_ptr);
				return rel_ptr;
			}

			template <typename RelPtr, size_t SlotSize = sizeof(RelPtr)>
			RelPtr enrel(any_ptr abs_ptr) {
				return enrel<RelPtr, SlotSize>(0, abs_ptr);
			}

			template <typename T = uint8_t>
			void reverse() {
				auto n = _this()->size() / sizeof(T);
				std::vector<T> r(n);
				T *p = _this()->base();
				for (size_t i = 0; i < n; ++i) {
					r[i] = p[n - 1 - i];
				}
				for (size_t i = 0; i < n; ++i) {
					p[i] = r[n - 1 - i];
				}
			}

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

						size_t eq(const Getter &gtr, size_t gtr_off = 0) const {
							return mem::get<Alignment>(&value) == gtr.template get<Alignment>(gtr_off) ? sizeof(Alignment) : 0;
						}

						size_t eq(const Getter &gtr, size_t gtr_off, size_t sz) const {
							switch (sz) {
								case 8:
									if (mem::get<uint64_t>(&value) == gtr.template get<uint64_t>(gtr_off)) {
										return sz;
									}
									return 0;

								////////////////////////////////////////////////////////////

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
										if (mem::get<uint8_t>(&value, i) != gtr.template get<uint8_t>(gtr_off + i)) {
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

					pattern(std::initializer_list<std::initializer_list<uint8_t>> byt_val_spls) {
						std::vector<uint16_t> byt_vals;

						auto sz = rua::nmax<size_t>();
						for (auto it = byt_val_spls.begin(); it != byt_val_spls.end(); ++it) {
							if (it->size() < sz) {
								sz = it->size();
							}
						}

						for (size_t i = 0; i < sz; ++i) {
							const uint8_t *b = nullptr;
							bool same = true;
							for (auto it = byt_val_spls.begin(); it != byt_val_spls.end(); ++it) {
								if (!b) {
									b = &it->begin()[i];
									continue;
								}
								if (*b != it->begin()[i]) {
									same = false;
									break;
								}
							}
							if (same) {
								byt_vals.emplace_back(*b);
							} else {
								byt_vals.emplace_back(1111);
							}
						}

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

						size_t eq(const Getter &gtr, size_t gtr_off = 0) const {
							if (is_void()) {
								return static_cast<size_t>(value);
							}
							return mem::get<Alignment>(&value) == (gtr.template get<Alignment>(gtr_off) & mask) ? sizeof(Alignment) : 0;
						}

						size_t eq(const Getter &gtr, size_t gtr_off, size_t sz) const {
							if (is_void()) {
								return value ? static_cast<size_t>(value) : sz;
							}

							switch (sz) {
								case 8:
									return
										mem::get<uint64_t>(&value) ==
										(
											gtr.template get<uint64_t>(gtr_off) &
											mem::get<uint64_t>(&mask)
										)
									;

								////////////////////////////////////////////////////////////

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

			static constexpr size_t npos = static_cast<size_t>(-1);

			struct search_result_t {
				size_t pos;

				operator bool() const {
					return pos != npos;
				}
			};

			template <typename Formatted>
			search_result_t search(const Formatted &byts) const {
				if (!byts.size() || _this()->size() < byts.size()) {
					return search_result_t{ npos };
				}

				size_t end = _this()->size() + 1 - byts.size();
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
									return search_result_t{ i };
								}
								sm_sz = 0;
							}
						}
					} else {
						for (size_t i = 0; i < end || !end; ++i) {
							if (byts.words().back().eq(*_this(), i + sm_sz, byts.size_remainder())) {
								return search_result_t{ i };
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
							return search_result_t{ i };
						}
					}
				}

				return search_result_t{ npos };
			}

			using find_result_t = search_result_t;

			template <typename Alignment>
			find_result_t find(const aligned<Alignment> &byts) const {
				return search(byts);
			}

			find_result_t find(const aligned<uintmax_t> &byts) const {
				return find<uintmax_t>(byts);
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

			template <typename Alignment>
			match_result_t match(const pattern<Alignment> &pat) const {
				auto sr = search(pat);
				if (!sr) {
					return match_result_t{ npos, {} };
				}
				return match_result_t{ sr.pos, pat.void_block_poss(sr.pos) };
			}

			match_result_t match(const pattern<uintmax_t> &pat) const {
				return match<uintmax_t>(pat);
			}

		private:
			Getter *_this() {
				return static_cast<Getter *>(this);
			}

			const Getter *_this() const {
				return static_cast<const Getter *>(this);
			}

		protected:
			bin_base() = default;
	};

	inline RUA_CONSTEXPR_14 size_t _strlen(const char *c_str) {
		size_t i = 0;
		while (c_str[i] != 0) {
			++i;
		}
		return i;
	}

	class bin_view : public bin_base<bin_view> {
		public:
			constexpr bin_view() : _base(nullptr), _sz(0) {}

			constexpr bin_view(std::nullptr_t) : bin_view() {}

			constexpr bin_view(const void *ptr, size_t sz = nmax<size_t>()) : _base(ptr), _sz(ptr ? sz : 0) {}

			template <
				typename T,
				typename = typename std::enable_if<
					!std::is_same<typename std::decay<T>::type, void>::value
				>::type
			>
			constexpr bin_view(const T *ptr, size_t sz = sizeof(T)) : bin_view(reinterpret_cast<const void *>(ptr), sz) {}

			template <
				typename T,
				typename = typename std::enable_if<
					!std::is_pointer<typename std::decay<T>::type>::value &&
					!std::is_convertible<T, std::string>::value &&
					!std::is_base_of<bin_view, typename std::decay<T>::type>::value
				>::type
			>
			constexpr bin_view(const T &ref, size_t sz = sizeof(T)) : bin_view(&ref, sz) {}

			RUA_CONSTEXPR_14 bin_view(const char *c_str) : _base(c_str), _sz(_strlen(c_str)) {}

			#if RUA_CPP >= RUA_CPP_17 && RUA_HAS_INC(<string_view>)
				constexpr bin_view(std::string_view sv) : _base(sv.data()), _sz(sv.length()) {}
			#else
				bin_view(const std::string &str) : _base(str.data()), _sz(str.length()) {}
			#endif

			operator bool() const {
				return _base;
			}

			any_ptr base() const {
				return _base;
			}

			size_t size() const {
				return _sz;
			}

			template <typename D>
			const D &get(ptrdiff_t offset = 0) const {
				return mem::get<D>(_base, offset);
			}

			template <typename D>
			const D &aligned_get(ptrdiff_t ix = 0) const {
				return get<D>(ix * sizeof(D));
			}

			const uint8_t &operator[](ptrdiff_t offset) const {
				return get<uint8_t>(offset);
			}

			bin_view slice(ptrdiff_t begin, ptrdiff_t end) const {
				assert(end > begin);
				assert(_sz <= static_cast<size_t>(nmax<ptrdiff_t>()));

				auto _szi = static_cast<ptrdiff_t>(_sz);
				if (end > _szi) {
					end = _szi;
				}
				return bin_view(_base + begin, static_cast<size_t>(end - begin));
			}

			bin_view slice(ptrdiff_t begin) const {
				assert(begin < static_cast<ptrdiff_t>(_sz));

				return bin_view(_base + begin, static_cast<size_t>(_sz - begin));
			}

			bin_view operator()(ptrdiff_t begin, ptrdiff_t end) const {
				return slice(begin, end);
			}

			bin_view operator()(ptrdiff_t begin) const {
				return slice(begin);
			}

			bin_view &slice_self(ptrdiff_t begin, ptrdiff_t end) & {
				assert(end > begin);

				auto _szi = static_cast<ptrdiff_t>(_sz);
				if (end > _szi) {
					end = _szi;
				}

				_base += begin;
				_sz = static_cast<size_t>(end - begin);

				return *this;
			}

			bin_view &&slice_self(ptrdiff_t begin, ptrdiff_t end) && {
				return std::move(slice_self(begin, end));
			}

			bin_view &slice_self(ptrdiff_t begin) & {
				_base += begin;
				_sz = static_cast<size_t>(_sz - begin);
				return *this;
			}

			bin_view &&slice_self(ptrdiff_t begin) && {
				return std::move(slice_self(begin));
			}

			void rebase(any_ptr ptr) {
				_base = ptr;
				if (!ptr) {
					_sz = 0;
				}
			}

			void resize(size_t sz = 0) {
				_sz = sz;
			}

		private:
			any_ptr _base;
			size_t _sz;

		protected:
			void _reset(void *ptr = nullptr, size_t sz = 0) {
				_base = ptr;
				_sz = ptr ? sz : 0;
			}
	};

	class bin_ref : public bin_view {
		public:
			constexpr bin_ref() : bin_view() {}

			constexpr bin_ref(std::nullptr_t) : bin_ref() {}

			constexpr bin_ref(void *ptr, size_t sz = nmax<size_t>()) : bin_view(ptr, sz) {}

			template <
				typename T,
				typename = typename std::enable_if<
					!std::is_const<T>::value &&
					!std::is_same<typename std::decay<T>::type, void>::value
				>::type
			>
			constexpr bin_ref(T *ptr, size_t sz = sizeof(T)) : bin_ref(reinterpret_cast<void *>(ptr), sz) {}

			template <
				typename T,
				typename = typename std::enable_if<
					!std::is_const<T>::value &&
					!std::is_pointer<typename std::decay<T>::type>::value &&
					!std::is_convertible<T, std::string>::value &&
					!std::is_base_of<bin_view, typename std::decay<T>::type>::value
				>::type
			>
			constexpr bin_ref(T &ref, size_t sz = sizeof(T)) : bin_ref(&ref, sz) {}

			RUA_CONSTEXPR_14 bin_ref(char *c_str) : bin_view(c_str) {}

			bin_ref(std::string &str) : bin_view(str.data(), str.length()) {}

			size_t copy(bin_view src) {
				auto cp_sz = size() < src.size() ? size() : src.size();
				if (!cp_sz) {
					return 0;
				}

				memcpy(base(), src.base(), cp_sz);
				return cp_sz;
			}

			template <typename D>
			D &get(ptrdiff_t offset = 0) {
				return mem::get<D>(base(), offset);
			}

			template <typename D>
			const D &get(ptrdiff_t offset = 0) const {
				return bin_view::template get<D>(offset);
			}

			template <typename D>
			D &aligned_get(ptrdiff_t ix = 0) {
				return get<D>(ix * sizeof(D));
			}

			template <typename D>
			const D &aligned_get(ptrdiff_t ix = 0) const {
				return bin_view::template aligned_get<D>(ix);
			}

			uint8_t &operator[](ptrdiff_t offset) {
				return get<uint8_t>(offset);
			}

			bin_ref slice(ptrdiff_t begin, ptrdiff_t end) {
				assert(end > begin);
				assert(size() <= static_cast<size_t>(nmax<ptrdiff_t>()));

				auto _szi = static_cast<ptrdiff_t>(size());
				if (end > _szi) {
					end = _szi;
				}
				return bin_ref(base() + begin, static_cast<size_t>(end - begin));
			}

			bin_view slice(ptrdiff_t begin, ptrdiff_t end) const {
				return bin_view::slice(begin, end);
			}

			bin_ref slice(ptrdiff_t begin) {
				assert(size() <= static_cast<size_t>(nmax<ptrdiff_t>()));

				return bin_ref(base() + begin, static_cast<size_t>(static_cast<ptrdiff_t>(size()) - begin));
			}

			bin_view slice(ptrdiff_t begin) const {
				return bin_view::slice(begin);
			}

			bin_ref operator()(ptrdiff_t begin, ptrdiff_t end) {
				return slice(begin, end);
			}

			bin_view operator()(ptrdiff_t begin, ptrdiff_t end) const {
				return bin_view::slice(begin, end);
			}

			bin_ref operator()(ptrdiff_t begin) {
				return slice(begin);
			}

			bin_view operator()(ptrdiff_t begin) const {
				return bin_view::slice(begin);
			}

			bin_ref &slice_self(ptrdiff_t begin, ptrdiff_t end) & {
				bin_view::slice_self(begin, end);
				return *this;
			}

			bin_ref &&slice_self(ptrdiff_t begin, ptrdiff_t end) && {
				return std::move(slice_self(begin, end));
			}

			bin_ref &slice_self(ptrdiff_t begin) & {
				bin_view::slice_self(begin);
				return *this;
			}

			bin_ref &&slice_self(ptrdiff_t begin) && {
				return std::move(slice_self(begin));
			}
	};

	class bin : public bin_ref {
		public:
			constexpr bin() : bin_ref(), _alloced(nullptr) {}

			constexpr bin(std::nullptr_t) : bin() {}

			explicit bin(size_t sz) {
				if (!sz) {
					return;
				}
				_alloc(sz);
				_reset(_alloced_base(), sz);
			}

			~bin() {
				reset();
			}

			bin(const bin_view &src) : bin(src.size()) {
				copy(src);
			}

			bin &operator=(const bin_view &src) {
				if (src) {
					reset(src.size());
					copy(src);
				} else {
					reset();
				}
				return *this;
			}

			bin(const bin &src) : bin(static_cast<const bin_view &>(src)) {}

			bin &operator=(const bin &src) {
				*this = static_cast<const bin_view &>(src);
				return *this;
			}

			template <
				typename T,
				typename = typename std::enable_if<
					!std::is_same<typename std::decay<T>::type, void>::value
				>::type
			>
			bin(const T *ptr, size_t sz = sizeof(T)) : bin(bin_view(reinterpret_cast<const void *>(ptr), sz)) {}

			template <
				typename T,
				typename = typename std::enable_if<
					!std::is_pointer<typename std::decay<T>::type>::value &&
					!std::is_convertible<T, std::string>::value &&
					!std::is_pointer<typename std::decay<T>::type>::value &&
					!std::is_base_of<bin_view, typename std::decay<T>::type>::value
				>::type
			>
			bin(T &ref, size_t sz = sizeof(T)) : bin(&ref, sz) {}

			bin(const char *c_str) : bin(bin_view(c_str)) {}

			#if RUA_CPP >= RUA_CPP_17 && RUA_HAS_INC(<string_view>)
				bin(std::string_view sv) : bin(bin_view(sv.data(), sv.length())) {}
			#else
				bin(const std::string &str) : bin(bin_view(str.data(), str.length())) {}
			#endif

			bin(bin &&src) : bin_ref(static_cast<bin_ref &&>(std::move(src))), _alloced(src._alloced) {
				if (src) {
					src._reset();
					src._alloced = nullptr;
				} else {
					_reset();
				}
			}

			bin &operator=(bin &&src) {
				if (src) {
					if (*this) {
						free(base());
					}
					_reset(src.base(), src.size());
					src._reset();
					src._alloced = nullptr;
				} else {
					reset();
				}
				return *this;
			}

			bin &slice_self(ptrdiff_t begin, ptrdiff_t end) & {
				assert(_alloced);

				bin_view::slice_self(begin, end);

				assert(base() >= _alloced_base());
				assert(base() < _alloced_base() + _alloced_size());
				assert(size() <= _alloced_size() - static_cast<size_t>(base() - _alloced_base()));

				return *this;
			}

			bin &&slice_self(ptrdiff_t begin, ptrdiff_t end) && {
				return std::move(slice_self(begin, end));
			}

			bin &slice_self(ptrdiff_t begin) & {
				assert(_alloced);

				bin_view::slice_self(begin);

				assert(base() >= _alloced_base());
				assert(base() < _alloced_base() + _alloced_size());
				assert(size() <= _alloced_size() - static_cast<size_t>(base() - _alloced_base()));

				return *this;
			}

			bin &&slice_self(ptrdiff_t begin) && {
				return std::move(slice_self(begin));
			}

			bool resize(size_t sz) {
				if (!sz) {
					reset();
					return true;
				}
				if (_alloced) {
					if (sz < _alloced_size() - static_cast<size_t>(base() - _alloced_base())) {
						bin_view::resize(sz);
						return false;
					}

					bin_view old_view = *this;
					auto old_alloced = _alloced;

					_alloc(sz);
					memcpy(base(), old_view.base(), old_view.size());

					free(old_alloced);
					return true;
				}
				_alloc(sz);
				return true;
			}

			void reset(size_t sz) {
				if (!sz) {
					reset();
					return;
				}
				if (_alloced) {
					if (sz <= _alloced_size()) {
						_reset(_alloced_base(), sz);
						return;
					}
					free(_alloced);
				}
				_alloc(sz);
			}

			void reset() {
				if (_alloced) {
					free(_alloced);
					_alloced = nullptr;
				}
				if (*this) {
					_reset();
				}
			}

		private:
			any_ptr _alloced;

			size_t &_alloced_size() {
				return *_alloced.to<size_t *>();
			}

			const size_t &_alloced_size() const {
				return *_alloced.to<const size_t *>();
			}

			any_ptr _alloced_base() const {
				return _alloced + sizeof(size_t);
			}

			void _alloc(size_t sz) {
				_alloced = malloc(sizeof(size_t) + sz);
				_alloced_size() = sz;
				_reset(_alloced_base(), sz);
			}
	};

	class bin_getter {
		public:
			template <typename T>
			T &get(ptrdiff_t offset = 0) {
				return mem::get<T>(this, offset);
			}

			template <typename T>
			T &aligned_get(ptrdiff_t ix = 0) {
				return get<T>(ix * sizeof(T));
			}

			template <typename T>
			const T &get(ptrdiff_t offset = 0) const {
				return mem::get<T>(this, offset);
			}

			template <typename T>
			const T &aligned_get(ptrdiff_t ix = 0) const {
				return get<T>(ix * sizeof(T));
			}
	};

	template <size_t Sz>
	class bin_block : public bin_getter, public bin_base<bin_block<Sz>> {
		public:
			any_ptr base() {
				return this;
			}

			static constexpr size_t size() {
				return Sz;
			}

		private:
			uint8_t _raw[Sz];
	};

	template <size_t Sz = nmax<size_t>()>
	using bin_block_ptr = bin_block<Sz> *;
}

#endif
