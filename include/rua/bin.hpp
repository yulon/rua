#ifndef _RUA_BIN_HPP
#define _RUA_BIN_HPP

#include "any_ptr.hpp"
#include "mem/get.hpp"

#include "macros.hpp"
#include "limits.hpp"

#ifdef __cpp_lib_string_view
	#include <string_view>
#endif

#include <string>
#include <cstdint>
#include <cstddef>
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

			template <typename CmpUnit = uintmax_t>
			class find_pattern {
				public:
					template <typename Container>
					find_pattern(const Container &byt_vals) {
						_input(byt_vals);
					}

					find_pattern(std::initializer_list<uint8_t> byt_vals) {
						_input(byt_vals);
					}

					size_t size() const {
						return _sz;
					}

					struct word {
						typename std::conditional<sizeof(CmpUnit) >= sizeof(size_t), CmpUnit, size_t>::type value;
						uint8_t size = sizeof(CmpUnit);

						size_t eq(const uint8_t *begin) const {
							if (size == sizeof(CmpUnit)) {
								return mem::get<CmpUnit>(&value) == mem::get<CmpUnit>(begin) ? sizeof(CmpUnit) : 0;
							}

							switch (size) {
								case 8:
									if (mem::get<uint64_t>(&value) == mem::get<uint64_t>(begin)) {
										return size;
									}
									return 0;

								////////////////////////////////////////////////////////////

								case 7:
									if (mem::get<uint8_t>(&value, 6) != mem::get<uint8_t>(begin, 6)) {
										return 0;
									}
									RUA_FALLTHROUGH;

								case 6:
									if (
										mem::get<uint32_t>(&value) == mem::get<uint32_t>(begin) &&
										mem::get<uint16_t>(&value, 4) == mem::get<uint16_t>(begin, 4)
									) {
										return size;
									}
									return 0;

								////////////////////////////////////////////////////////////

								case 5:
									if (mem::get<uint8_t>(&value, 4) != mem::get<uint8_t>(begin, 4)) {
										return 0;
									}
									RUA_FALLTHROUGH;

								case 4:
									return mem::get<uint32_t>(&value) == mem::get<uint32_t>(begin);

								////////////////////////////////////////////////////////////

								case 3:
									if (mem::get<uint8_t>(&value, 2) != mem::get<uint8_t>(begin, 2)) {
										return 0;
									}
									RUA_FALLTHROUGH;

								case 2:
									return mem::get<uint16_t>(&value) == mem::get<uint16_t>(begin);

								////////////////////////////////////////////////////////////

								case 1:
									return mem::get<uint8_t>(&value) == mem::get<uint8_t>(begin);

								////////////////////////////////////////////////////////////

								default:
									for (size_t i = 0; i < size; ++i) {
										if (mem::get<uint8_t>(&value, i) != mem::get<uint8_t>(begin, i)) {
											return 0;
										}
									}
							}

							return size;
						}
					};

					const std::vector<word> &words() const {
						return _words;
					}

					size_t hash() const {
						return _h;
					}

					void calc_hash(size_t &h, const uint8_t *begin, bool is_update = false) const {
						auto end = begin + _sz;
						if (is_update) {
							h -= *(--begin);
							h += *(--end);
							return;
						}
						h = 0;
						for (auto it = begin; it != end; ++it) {
							h += *it;
						}
					}

				private:
					size_t _sz;
					std::vector<word> _words;
					size_t _h;

					template <typename Container>
					void _input(const Container &byt_vals) {
						_sz = byt_vals.size();

						if (!_sz) {
							return;
						}

						auto sz_rmdr = byt_vals.size() % sizeof(CmpUnit);

						if (sz_rmdr) {
							_words.resize(byt_vals.size() / sizeof(CmpUnit) + 1);
						} else {
							_words.resize(byt_vals.size() / sizeof(CmpUnit));
						}

						size_t wi = 0;
						size_t last_sz = 0;
						_h = 0;

						for (size_t i = 0; i < _sz; ++i) {
							if (last_sz == sizeof(CmpUnit)) {
								last_sz = 0;
								++wi;
							}
							mem::get<uint8_t>(&_words[wi].value, last_sz) = static_cast<uint8_t>(byt_vals.begin()[i]);
							++last_sz;

							_h += static_cast<size_t>(static_cast<uint8_t>(byt_vals.begin()[i]));
						}

						if (sz_rmdr) {
							_words.back().size = static_cast<uint8_t>(sz_rmdr);
						}
					}
			};

			template <typename CmpUnit = uintmax_t>
			class match_pattern {
				public:
					template <typename Container>
					match_pattern(const Container &byt_vals) {
						_input(byt_vals);
					}

					match_pattern(std::initializer_list<uint16_t> byt_vals) {
						_input(byt_vals);
					}

					match_pattern(std::initializer_list<std::initializer_list<uint8_t>> byt_val_spls) {
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
						CmpUnit mask = 0;
						typename std::conditional<sizeof(CmpUnit) >= sizeof(size_t), CmpUnit, size_t>::type value = 0;
						uint8_t size = sizeof(CmpUnit);

						bool is_void() const {
							return !mask;
						}

						size_t eq(const uint8_t *target) const {
							if (size == sizeof(CmpUnit)) {
								if (is_void()) {
									return size;
								}
								return mem::get<CmpUnit>(&value) == (mem::get<CmpUnit>(target) & mask) ? sizeof(CmpUnit) : 0;
							}

							if (is_void()) {
								return size;
							}

							switch (size) {
								case 8:
									return
										mem::get<uint64_t>(&value) ==
										(
											mem::get<uint64_t>(target) &
											mem::get<uint64_t>(&mask)
										)
									;

								////////////////////////////////////////////////////////////

								case 7:
									if (
										mem::get<uint8_t>(&value, 6) !=
										(
											mem::get<uint8_t>(target + 6) &
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
											mem::get<uint32_t>(target) &
											mem::get<uint32_t>(&mask)
										) &&
										mem::get<uint16_t>(&value, 4) ==
										(
											mem::get<uint16_t>(target + 4) &
											mem::get<uint16_t>(&mask, 4)
										)
									) {
										return size;
									}
									return 0;

								////////////////////////////////////////////////////////////

								case 5:
									if (
										mem::get<uint8_t>(&value, 4) !=
										(
											mem::get<uint8_t>(target + 4) &
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
											mem::get<uint32_t>(target) &
											mem::get<uint32_t>(&mask)
										)
									;

								////////////////////////////////////////////////////////////

								case 3:
									if (
										mem::get<uint8_t>(&value, 2) !=
										(
											mem::get<uint8_t>(target + 2) &
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
											mem::get<uint16_t>(target) &
											mem::get<uint16_t>(&mask)
										)
									;

								////////////////////////////////////////////////////////////

								case 1:
									return
										mem::get<uint8_t>(&value) ==
										(
											mem::get<uint8_t>(target) &
											mem::get<uint8_t>(&mask)
										)
									;

								////////////////////////////////////////////////////////////

								default:
									for (size_t i = 0; i < size; ++i) {
										if (
											mem::get<uint8_t>(&value, i) !=
											(
												mem::get<uint8_t>(target + i) &
												mem::get<uint8_t>(&mask, i)
											)
										) {
											return 0;
										}
									}
							}

							return size;
						}
					};

					const std::vector<word> &words() const {
						return _words;
					}

					static constexpr size_t hash() {
						return 0;
					}

					static void calc_hash(size_t &h, const uint8_t *begin, bool is_update = false) {

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
					size_t _h;
					std::vector<word> _words;
					std::vector<size_t> _void_block_poss;

					template <typename Container>
					void _input(const Container &byt_vals) {
						_sz = byt_vals.size();

						if (!_sz) {
							return;
						}

						bool in_void = false;
						size_t last_sz = sizeof(CmpUnit);

						for (size_t i = 0; i < _sz; ++i) {
							if (last_sz == sizeof(CmpUnit)) {
								last_sz = 0;
								_words.emplace_back();
							}
							if (byt_vals.begin()[i] < 256) {
								if (in_void) {
									in_void = false;
								}
								mem::get<uint8_t>(&_words.back().mask, last_sz) = 255;
								mem::get<uint8_t>(&_words.back().value, last_sz) = static_cast<uint8_t>(byt_vals.begin()[i]);
							} else {
								if (!in_void) {
									in_void = true;
									_void_block_poss.emplace_back(i);
								}
							}
							++last_sz;
						}

						auto sz_rmdr = byt_vals.size() % sizeof(CmpUnit);
						if (sz_rmdr) {
							_words.back().size = static_cast<uint8_t>(sz_rmdr);
						}
					}
			};

			static constexpr size_t npos = nmax<size_t>();

			struct search_result {
				size_t pos;

				operator bool() const {
					return pos != npos;
				}
			};

			template <typename Pattern>
			search_result search(const Pattern &byts) const {
				if (!byts.size() || _this()->size() < byts.size()) {
					return search_result{ npos };
				}

				auto begin = &_this()->template get<uint8_t>();
				const uint8_t *end = begin + _this()->size() - byts.size();

				size_t h = 0;
				const uint8_t *cmp_ptr = nullptr;

				for (auto it = begin; it != end; ++it) {
					byts.calc_hash(
						h,
						it,
						it != begin
					);
					if (h == byts.hash()) {
						cmp_ptr = it;
						for (auto &wd : byts.words()) {
							size_t sz = wd.eq(cmp_ptr);
							if (!sz) {
								cmp_ptr = nullptr;
								break;
							}
							cmp_ptr += sz;
						}
						if (cmp_ptr) {
							return search_result{ static_cast<size_t>(it - begin) };
						}
					}
				}

				return search_result{ npos };
			}

			using find_result = search_result;

			template <typename CmpUnit>
			find_result find(const find_pattern<CmpUnit> &byts) const {
				return search(byts);
			}

			find_result find(const find_pattern<uintmax_t> &byts) const {
				return find<uintmax_t>(byts);
			}

			struct match_result {
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

			template <typename CmpUnit>
			match_result match(const match_pattern<CmpUnit> &pat) const {
				auto sr = search(pat);
				if (!sr) {
					return match_result{ npos, {} };
				}
				return match_result{ sr.pos, pat.void_block_poss(sr.pos) };
			}

			match_result match(const match_pattern<uintmax_t> &pat) const {
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

	inline RUA_CONSTEXPR_14 size_t strlen(const char *c_str) {
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

			constexpr bin_view(const void *ptr, size_t sz = nmax<size_t>()) : _base(sz ? ptr : nullptr), _sz(ptr ? sz : 0) {}

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
					!std::is_convertible<T, any_ptr>::value &&
					!std::is_convertible<T, std::string>::value &&
					!std::is_base_of<bin_view, typename std::decay<T>::type>::value
				>::type
			>
			constexpr bin_view(const T &ref, size_t sz = sizeof(T)) : bin_view(&ref, sz) {}

			RUA_CONSTEXPR_14 bin_view(const char *c_str) : _base(c_str), _sz(strlen(c_str)) {}

			bin_view(const std::string &str) : _base(str.data()), _sz(str.length()) {}

			#ifdef __cpp_lib_string_view
				constexpr bin_view(std::string_view sv) : _base(sv.data()), _sz(sv.length()) {}
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
				return *reinterpret_cast<D *>(_base.value() + offset);
			}

			template <typename D>
			const D &aligned_get(ptrdiff_t ix = 0) const {
				return *reinterpret_cast<D *>(_base.value() + ix * sizeof(D));
			}

			const uint8_t &operator[](ptrdiff_t offset) const {
				return get<uint8_t>(offset);
			}

			bin_view slice(ptrdiff_t begin, ptrdiff_t end) const {
				assert(end > begin);
				assert(size() <= static_cast<size_t>(nmax<ptrdiff_t>()));

				auto szi = static_cast<ptrdiff_t>(size());
				if (end > szi) {
					end = szi;
				}
				auto new_sz = static_cast<size_t>(end - begin);
				if (!new_sz) {
					return nullptr;
				}
				return bin_view(_base + begin, static_cast<size_t>(end - begin));
			}

			bin_view slice(ptrdiff_t begin) const {
				assert(begin < static_cast<ptrdiff_t>(_sz));

				auto new_sz = static_cast<size_t>(_sz - begin);
				if (!new_sz) {
					return nullptr;
				}
				return bin_view(_base + begin, new_sz);
			}

			bin_view operator()(ptrdiff_t begin, ptrdiff_t end) const {
				return slice(begin, end);
			}

			bin_view operator()(ptrdiff_t begin) const {
				return slice(begin);
			}

			bin_view &slice_self(ptrdiff_t begin, ptrdiff_t end) & {
				assert(end > begin);

				auto szi = static_cast<ptrdiff_t>(_sz);
				if (end > szi) {
					end = szi;
				}

				_sz = static_cast<size_t>(end - begin);
				if (_sz) {
					_base += begin;
				} else {
					_base = nullptr;
				}

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

		protected:
			any_ptr _base;
			size_t _sz;

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
					!std::is_convertible<T, any_ptr>::value &&
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

				memcpy(_base, src.base(), cp_sz);
				return cp_sz;
			}

			template <typename D>
			D &get(ptrdiff_t offset = 0) {
				return *reinterpret_cast<D *>(_base.value() + offset);
			}

			template <typename D>
			const D &get(ptrdiff_t offset = 0) const {
				return *reinterpret_cast<D *>(_base.value() + offset);
			}

			template <typename D>
			D &aligned_get(ptrdiff_t ix = 0) {
				return *reinterpret_cast<D *>(_base.value() + ix * sizeof(D));
			}

			template <typename D>
			const D &aligned_get(ptrdiff_t ix = 0) const {
				return *reinterpret_cast<D *>(_base.value() + ix * sizeof(D));
			}

			uint8_t &operator[](ptrdiff_t offset) {
				return get<uint8_t>(offset);
			}

			bin_ref slice(ptrdiff_t begin, ptrdiff_t end) {
				auto v = bin_view::slice(begin, end);
				return bin_ref(v.base(), v.size());
			}

			bin_view slice(ptrdiff_t begin, ptrdiff_t end) const {
				return bin_view::slice(begin, end);
			}

			bin_ref slice(ptrdiff_t begin) {
				auto v = bin_view::slice(begin);
				return bin_ref(v.base(), v.size());
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
					!std::is_convertible<T, any_ptr>::value &&
					!std::is_convertible<T, std::string>::value &&
					!std::is_base_of<bin_view, typename std::decay<T>::type>::value
				>::type
			>
			bin(T &ref, size_t sz = sizeof(T)) : bin(&ref, sz) {}

			bin(const char *c_str) : bin(bin_view(c_str)) {}

			bin(const std::string &str) : bin(bin_view(str.data(), str.length())) {}

			#ifdef __cpp_lib_string_view
				bin(std::string_view sv) : bin(bin_view(sv.data(), sv.length())) {}
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
						free(_base);
					}
					_reset(src._base, src.size());
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

				if (!*this) {
					_free_alloced();
					return *this;
				}

				assert(_base >= _alloced_base());
				assert(_base < _alloced_base() + _alloced_size());
				assert(size() <= _alloced_size() - static_cast<size_t>(_base - _alloced_base()));

				return *this;
			}

			bin &&slice_self(ptrdiff_t begin, ptrdiff_t end) && {
				return std::move(slice_self(begin, end));
			}

			bin &slice_self(ptrdiff_t begin) & {
				assert(_alloced);

				bin_view::slice_self(begin);

				if (!*this) {
					_free_alloced();
					return *this;
				}

				assert(_base >= _alloced_base());
				assert(_base < _alloced_base() + _alloced_size());
				assert(size() <= _alloced_size() - static_cast<size_t>(_base - _alloced_base()));

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
					if (sz < _alloced_size() - static_cast<size_t>(_base - _alloced_base())) {
						bin_view::resize(sz);
						return false;
					}

					bin_view old_view = *this;
					auto old_alloced = _alloced;

					_alloc(sz);
					memcpy(_base, old_view.base(), old_view.size());

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
				_free_alloced();
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

			void _free_alloced() {
				if (_alloced) {
					free(_alloced);
					_alloced = nullptr;
				}
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
