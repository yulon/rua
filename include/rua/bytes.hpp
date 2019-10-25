#ifndef _RUA_BYTES_HPP
#define _RUA_BYTES_HPP

#include "bit.hpp"
#include "byte.hpp"
#include "limits.hpp"
#include "macros.hpp"
#include "string/str_len.hpp"
#include "string/string_view.hpp"
#include "type_traits.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace rua {

class bytes_view;
class bytes_ref;

template <typename Span>
class const_bytes_base {
public:
	template <typename T>
	RUA_FORCE_INLINE T get() const {
		return bit_get<T>(_this()->data());
	}

	template <typename T>
	RUA_FORCE_INLINE T get(ptrdiff_t offset) const {
		return bit_get<T>(_this()->data(), offset);
	}

	template <typename T>
	RUA_FORCE_INLINE T get_aligned(ptrdiff_t ix) const {
		return bit_get_aligned<T>(_this()->data(), ix);
	}

	template <typename T>
	RUA_FORCE_INLINE const T &at() const {
		return *_this()->data().template as<const T *>();
	}

	template <typename T>
	RUA_FORCE_INLINE const T &at(ptrdiff_t offset) const {
		return *(_this()->data() + offset).template as<const T *>();
	}

	template <typename T>
	RUA_FORCE_INLINE const T &at_aligned(ptrdiff_t ix) const {
		return _this()->data().template as<const T *>()[ix];
	}

	RUA_FORCE_INLINE const byte &operator[](ptrdiff_t offset) const {
		return *at<const byte *>(offset);
	}

	inline bytes_view
	slice(ptrdiff_t begin_offset, ptrdiff_t end_offset_from_begin) const;

	inline bytes_view slice(ptrdiff_t begin_offset) const;

	inline bytes_view
	operator()(ptrdiff_t begin_offset, ptrdiff_t end_offset_from_begin) const;

	inline bytes_view operator()(ptrdiff_t begin_offset) const;

	template <typename... DestArgs>
	inline size_t copy_to(DestArgs &&... dest) const;

	template <typename CharT, typename Traits>
	RUA_FORCE_INLINE operator basic_string_view<CharT, Traits>() const {
		return basic_string_view<CharT, Traits>(
			_this()->data().template as<const CharT *>(),
			_this()->size() / sizeof(CharT));
	}

	template <typename CharT, typename Traits, typename Allocator>
	RUA_FORCE_INLINE
	operator std::basic_string<CharT, Traits, Allocator>() const {
		return std::basic_string<CharT, Traits, Allocator>(
			_this()->data().template as<const CharT *>(),
			_this()->size() / sizeof(CharT));
	}

	template <typename RelPtr, size_t SlotSize = sizeof(RelPtr)>
	generic_ptr derel(ptrdiff_t pos = 0) const {
		return _this()->data() + pos + SlotSize + get<RelPtr>(pos);
	}

	template <typename CmpUnit = uintmax_t>
	class find_pattern {
	public:
		template <typename Container>
		find_pattern(const Container &byt_vals) {
			_input(byt_vals);
		}

		find_pattern(std::initializer_list<byte> byt_vals) {
			_input(byt_vals);
		}

		size_t size() const {
			return _n;
		}

		struct word {
			conditional_t<sizeof(CmpUnit) >= sizeof(size_t), CmpUnit, size_t>
				value;
			byte size = sizeof(CmpUnit);

			RUA_FORCE_INLINE size_t eq(const byte *begin) const {
				if (size == sizeof(CmpUnit)) {
					return bit_get<CmpUnit>(&value) == bit_get<CmpUnit>(begin)
							   ? sizeof(CmpUnit)
							   : 0;
				}

				switch (size) {
				case 8:
					if (bit_get<uint64_t>(&value) == bit_get<uint64_t>(begin)) {
						return size;
					}
					return 0;

					////////////////////////////////////////////////////////////

				case 7:
					if (bit_get<byte>(&value, 6) != bit_get<byte>(begin, 6)) {
						return 0;
					}
					RUA_FALLTHROUGH;

				case 6:
					if (bit_get<uint32_t>(&value) == bit_get<uint32_t>(begin) &&
						bit_get<uint16_t>(&value, 4) ==
							bit_get<uint16_t>(begin, 4)) {
						return size;
					}
					return 0;

					////////////////////////////////////////////////////////////

				case 5:
					if (bit_get<byte>(&value, 4) != bit_get<byte>(begin, 4)) {
						return 0;
					}
					RUA_FALLTHROUGH;

				case 4:
					return bit_get<uint32_t>(&value) ==
						   bit_get<uint32_t>(begin);

					////////////////////////////////////////////////////////////

				case 3:
					if (bit_get<byte>(&value, 2) != bit_get<byte>(begin, 2)) {
						return 0;
					}
					RUA_FALLTHROUGH;

				case 2:
					return bit_get<uint16_t>(&value) ==
						   bit_get<uint16_t>(begin);

					////////////////////////////////////////////////////////////

				case 1:
					return bit_get<byte>(&value) == bit_get<byte>(begin);

					////////////////////////////////////////////////////////////

				default:
					for (size_t i = 0; i < size; ++i) {
						if (bit_get<byte>(&value, i) !=
							bit_get<byte>(begin, i)) {
							return 0;
						}
					}
				}

				return size;
			}
		};

		RUA_FORCE_INLINE const std::vector<word> &words() const {
			return _words;
		}

		RUA_FORCE_INLINE size_t hash() const {
			return _h;
		}

		RUA_FORCE_INLINE void
		calc_hash(size_t &h, const byte *begin, bool is_update = false) const {
			auto end = begin + _n;
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
		size_t _n;
		std::vector<word> _words;
		size_t _h;

		template <typename Container>
		void _input(const Container &byt_vals) {
			_n = byt_vals.size();

			if (!_n) {
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

			for (size_t i = 0; i < _n; ++i) {
				if (last_sz == sizeof(CmpUnit)) {
					last_sz = 0;
					++wi;
				}
				bit_set<byte>(
					&_words[wi].value,
					last_sz,
					static_cast<byte>(byt_vals.begin()[i]));
				++last_sz;

				_h +=
					static_cast<size_t>(static_cast<byte>(byt_vals.begin()[i]));
			}

			if (sz_rmdr) {
				_words.back().size = static_cast<byte>(sz_rmdr);
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

		match_pattern(
			std::initializer_list<std::initializer_list<byte>> byt_val_spls) {
			std::vector<uint16_t> byt_vals;

			auto sz = rua::nmax<size_t>();
			for (auto it = byt_val_spls.begin(); it != byt_val_spls.end();
				 ++it) {
				if (it->size() < sz) {
					sz = it->size();
				}
			}

			for (size_t i = 0; i < sz; ++i) {
				const byte *b = nullptr;
				bool same = true;
				for (auto it = byt_val_spls.begin(); it != byt_val_spls.end();
					 ++it) {
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
			return _n;
		}

		struct word {
			CmpUnit mask = 0;
			conditional_t<sizeof(CmpUnit) >= sizeof(size_t), CmpUnit, size_t>
				value = 0;
			byte size = sizeof(CmpUnit);

			bool is_void() const {
				return !mask;
			}

			RUA_FORCE_INLINE size_t eq(const byte *target) const {
				if (size == sizeof(CmpUnit)) {
					if (is_void()) {
						return size;
					}
					return bit_get<CmpUnit>(&value) ==
								   (bit_get<CmpUnit>(target) & mask)
							   ? sizeof(CmpUnit)
							   : 0;
				}

				if (is_void()) {
					return size;
				}

				switch (size) {
				case 8:
					return bit_get<uint64_t>(&value) ==
						   (bit_get<uint64_t>(target) &
							bit_get<uint64_t>(&mask));

					////////////////////////////////////////////////////////////

				case 7:
					if (bit_get<byte>(&value, 6) !=
						(bit_get<byte>(target + 6) & bit_get<byte>(&mask, 6))) {
						return 0;
					}
					RUA_FALLTHROUGH;

				case 6:
					if (bit_get<uint32_t>(&value) ==
							(bit_get<uint32_t>(target) &
							 bit_get<uint32_t>(&mask)) &&
						bit_get<uint16_t>(&value, 4) ==
							(bit_get<uint16_t>(target + 4) &
							 bit_get<uint16_t>(&mask, 4))) {
						return size;
					}
					return 0;

					////////////////////////////////////////////////////////////

				case 5:
					if (bit_get<byte>(&value, 4) !=
						(bit_get<byte>(target + 4) & bit_get<byte>(&mask, 4))) {
						return 0;
					}
					RUA_FALLTHROUGH;

				case 4:
					return bit_get<uint32_t>(&value) ==
						   (bit_get<uint32_t>(target) &
							bit_get<uint32_t>(&mask));

					////////////////////////////////////////////////////////////

				case 3:
					if (bit_get<byte>(&value, 2) !=
						(bit_get<byte>(target + 2) & bit_get<byte>(&mask, 2))) {
						return 0;
					}
					RUA_FALLTHROUGH;

				case 2:
					return bit_get<uint16_t>(&value) ==
						   (bit_get<uint16_t>(target) &
							bit_get<uint16_t>(&mask));

					////////////////////////////////////////////////////////////

				case 1:
					return bit_get<byte>(&value) ==
						   (bit_get<byte>(target) & bit_get<byte>(&mask));

					////////////////////////////////////////////////////////////

				default:
					for (size_t i = 0; i < size; ++i) {
						if (bit_get<byte>(&value, i) !=
							(bit_get<byte>(target + i) &
							 bit_get<byte>(&mask, i))) {
							return 0;
						}
					}
				}

				return size;
			}
		};

		RUA_FORCE_INLINE const std::vector<word> &words() const {
			return _words;
		}

		static RUA_FORCE_INLINE constexpr size_t hash() {
			return 0;
		}

		static RUA_FORCE_INLINE void
		calc_hash(size_t &, const byte *, bool = false) {}

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
		size_t _n;
		size_t _h;
		std::vector<word> _words;
		std::vector<size_t> _void_block_poss;

		template <typename Container>
		void _input(const Container &byt_vals) {
			_n = byt_vals.size();

			if (!_n) {
				return;
			}

			bool in_void = false;
			size_t last_sz = sizeof(CmpUnit);

			for (size_t i = 0; i < _n; ++i) {
				if (last_sz == sizeof(CmpUnit)) {
					last_sz = 0;
					_words.emplace_back();
				}
				if (byt_vals.begin()[i] < 256) {
					if (in_void) {
						in_void = false;
					}
					bit_set<byte>(&_words.back().mask, last_sz, 255);
					bit_set<byte>(
						&_words.back().value,
						last_sz,
						static_cast<byte>(byt_vals.begin()[i]));
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
				_words.back().size = static_cast<byte>(sz_rmdr);
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
			return search_result{npos};
		}

		const byte *begin = _this()->data();
		auto end = begin + _this()->size() - byts.size();

		size_t h = 0;
		const byte *cmp_ptr = nullptr;

		for (auto it = begin; it != end; ++it) {
			byts.calc_hash(h, it, it != begin);
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
					return search_result{static_cast<size_t>(it - begin)};
				}
			}
		}

		return search_result{npos};
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
			return match_result{npos, {}};
		}
		return match_result{sr.pos, pat.void_block_poss(sr.pos)};
	}

	match_result match(const match_pattern<uintmax_t> &pat) const {
		return match<uintmax_t>(pat);
	}

protected:
	const_bytes_base() = default;

private:
	RUA_FORCE_INLINE const Span *_this() const {
		return static_cast<const Span *>(this);
	}
};

template <typename Span>
class bytes_base : public const_bytes_base<Span> {
public:
	template <typename T>
	RUA_FORCE_INLINE void set(const T &val) {
		return bit_set<T>(_this()->data(), val);
	}

	template <typename T>
	RUA_FORCE_INLINE void set(const T &val, ptrdiff_t offset) {
		return bit_set<T>(_this()->data(), offset, val);
	}

	template <typename T>
	RUA_FORCE_INLINE void set_aligned(const T &val, ptrdiff_t ix) {
		return bit_set_aligned<T>(_this()->data(), val, ix);
	}

	template <typename T>
	RUA_FORCE_INLINE const T &at() const {
		return *reinterpret_cast<const T *>(_this()->data());
	}

	template <typename T>
	RUA_FORCE_INLINE T &at() {
		return *reinterpret_cast<T *>(_this()->data());
	}

	template <typename T>
	RUA_FORCE_INLINE const T &at(ptrdiff_t offset) const {
		return *(_this()->data() + offset).template as<const T *>();
	}

	template <typename T>
	RUA_FORCE_INLINE T &at(ptrdiff_t offset) {
		return *(_this()->data() + offset).template as<T *>();
	}

	template <typename T>
	RUA_FORCE_INLINE const T &at_aligned(ptrdiff_t ix) const {
		return _this()->data().template as<const T *>()[ix];
	}

	template <typename T>
	RUA_FORCE_INLINE T &at_aligned(ptrdiff_t ix) {
		return _this()->data().template as<T *>()[ix];
	}

	RUA_FORCE_INLINE const byte &operator[](ptrdiff_t offset) const {
		return *at<const byte *>(offset);
		;
	}

	RUA_FORCE_INLINE byte &operator[](ptrdiff_t offset) {
		return *at<byte *>(offset);
		;
	}

	inline bytes_view
	slice(ptrdiff_t begin_offset, ptrdiff_t end_offset_from_begin) const;

	inline bytes_view slice(ptrdiff_t begin_offset) const;

	inline bytes_ref
	slice(ptrdiff_t begin_offset, ptrdiff_t end_offset_from_begin);

	inline bytes_ref slice(ptrdiff_t begin_offset);

	inline bytes_view
	operator()(ptrdiff_t begin_offset, ptrdiff_t end_offset_from_begin) const;

	inline bytes_view operator()(ptrdiff_t begin_offset) const;

	inline bytes_ref
	operator()(ptrdiff_t begin_offset, ptrdiff_t end_offset_from_begin);

	inline bytes_ref operator()(ptrdiff_t begin_offset);

	template <typename... SrcArgs>
	inline size_t copy_from(SrcArgs &&... src) const;

	template <typename RelPtr, size_t SlotSize = sizeof(RelPtr)>
	RelPtr enrel(generic_ptr abs_ptr, ptrdiff_t pos = 0) {
		auto rel_ptr =
			static_cast<RelPtr>(abs_ptr - (_this()->data() + pos + SlotSize));
		this->template set<RelPtr>(pos, rel_ptr);
		return rel_ptr;
	}

	template <typename T = byte>
	void reverse() {
		auto n = _this()->size() / sizeof(T);
		std::vector<T> r(n);
		auto p = reinterpret_cast<T *>(_this()->data());
		for (size_t i = 0; i < n; ++i) {
			r[i] = p[n - 1 - i];
		}
		for (size_t i = 0; i < n; ++i) {
			p[i] = r[n - 1 - i];
		}
	}

private:
	RUA_FORCE_INLINE Span *_this() {
		return static_cast<Span *>(this);
	}

	RUA_FORCE_INLINE const Span *_this() const {
		return static_cast<const Span *>(this);
	}
};

class bytes_view : public const_bytes_base<bytes_view> {
public:
	constexpr bytes_view() : _p(nullptr), _n(0) {}

	constexpr bytes_view(std::nullptr_t) : bytes_view() {}

	constexpr bytes_view(generic_ptr ptr, size_t size) :
		_p(ptr),
		_n(ptr ? size : 0) {}

	bytes_view(const byte *ptr, size_t size) :
		_p(reinterpret_cast<const char *>(ptr)),
		_n(ptr ? size : 0) {}

	bytes_view(const char *c_str, size_t size) : _p(c_str), _n(size) {}

	bytes_view(const char *c_str) : _p(c_str), _n(c_str ? str_len(c_str) : 0) {}

	bytes_view(const wchar_t *c_wstr, size_t size) :
		_p(reinterpret_cast<const char *>(c_wstr)),
		_n(size) {}

	bytes_view(const wchar_t *c_wstr) :
		_p(reinterpret_cast<const char *>(c_wstr)),
		_n(c_wstr ? str_len(c_wstr) * sizeof(wchar_t) : 0) {}

	template <
		typename T,
		typename = enable_if_t<
			!std::is_same<T, const byte>::value &&
			!std::is_convertible<T *, string_view>::value &&
			!std::is_convertible<T *, wstring_view>::value &&
			!std::is_function<T>::value>>
	bytes_view(T *ptr, size_t size = sizeof(T)) :
		_p(reinterpret_cast<const char *>(ptr)),
		_n(ptr ? size : 0) {}

	template <typename R, typename... A>
	bytes_view(R (*ptr)(A...), size_t size = nmax<size_t>()) :
		_p(reinterpret_cast<const char *>(ptr)),
		_n(size) {}

	template <
		typename T,
		typename = enable_if_t<
			!std::is_convertible<T &&, string_view>::value &&
			!std::is_convertible<T &&, wstring_view>::value &&
			!is_span<T &&>::value>>
	bytes_view(T &&ref, size_t size = sizeof(T)) : bytes_view(&ref, size) {}

	template <
		typename T,
		typename SpanTraits = span_traits<T &&>,
		typename = enable_if_t<
			!std::is_base_of<bytes_view, decay_t<T>>::value &&
			(!std::is_array<remove_reference_t<T>>::value ||
			 (!std::is_same<typename SpanTraits::value_type, char>::value &&
			  !std::is_same<typename SpanTraits::value_type, wchar_t>::value))>>
	bytes_view(T &&span) :
		bytes_view(
			rua::data(std::forward<T>(span)),
			rua::size(std::forward<T>(span)) *
				sizeof(typename SpanTraits::element_type)) {}

	constexpr operator bool() const {
		return _p;
	}

	generic_ptr data() const {
		return _p;
	}

	constexpr size_t size() const {
		return _n;
	}

	void resize(size_t size = 0) {
		if (!size) {
			reset();
			return;
		}
		if (!_p) {
			return;
		}
		_n = size;
	}

	void reset() {
		if (!_p) {
			return;
		}
		_p = nullptr;
		_n = 0;
	}

	template <typename... Args>
	void reset(Args &&... args) {
		RUA_SASSERT((std::is_constructible<bytes_view, Args...>::value));

		*this = bytes_view(std::forward<Args>(args)...);
	}

private:
	generic_ptr _p;
	size_t _n;
};

class bytes_ref : public bytes_base<bytes_ref> {
public:
	constexpr bytes_ref() : _p(nullptr), _n(0) {}

	constexpr bytes_ref(std::nullptr_t) : bytes_ref() {}

	constexpr bytes_ref(generic_ptr ptr, size_t size) :
		_p(ptr),
		_n(ptr ? size : 0) {}

	bytes_ref(byte *ptr, size_t size) :
		_p(reinterpret_cast<char *>(ptr)),
		_n(ptr ? size : 0) {}

	bytes_ref(char *c_str, size_t size) : _p(c_str), _n(size) {}

	bytes_ref(char *c_str) : _p(c_str), _n(c_str ? str_len(c_str) : 0) {}

	bytes_ref(wchar_t *c_wstr, size_t size) :
		_p(reinterpret_cast<char *>(c_wstr)),
		_n(size) {}

	bytes_ref(wchar_t *c_wstr) :
		_p(reinterpret_cast<char *>(c_wstr)),
		_n(str_len(c_wstr) * sizeof(wchar_t)) {}

	template <
		typename T,
		typename = enable_if_t<
			!std::is_const<T>::value && !std::is_same<T, byte>::value &&
			!std::is_convertible<T *, string_view>::value &&
			!std::is_convertible<T *, wstring_view>::value &&
			!std::is_function<T>::value>>
	bytes_ref(T *ptr, size_t size = sizeof(T)) :
		_p(reinterpret_cast<char *>(ptr)),
		_n(ptr ? size : 0) {}

	template <
		typename T,
		typename = enable_if_t<
			!std::is_const<remove_reference_t<T>>::value &&
			!std::is_convertible<T &&, string_view>::value &&
			!std::is_convertible<T &&, wstring_view>::value &&
			!is_span<T &&>::value>>
	bytes_ref(T &&ref, size_t size = sizeof(T)) : bytes_ref(&ref, size) {}

	template <
		typename T,
		typename SpanTraits = span_traits<T &&>,
		typename = enable_if_t<
			!std::is_base_of<bytes_ref, decay_t<T>>::value &&
			!std::is_const<typename SpanTraits::element_type>::value &&
			(!std::is_array<remove_reference_t<T>>::value ||
			 (!std::is_same<typename SpanTraits::value_type, char>::value &&
			  !std::is_same<typename SpanTraits::value_type, wchar_t>::value))>>
	bytes_ref(T &&span) :
		bytes_ref(
			rua::data(std::forward<T>(span)),
			rua::size(std::forward<T>(span)) *
				sizeof(typename SpanTraits::element_type)) {}

	constexpr operator bool() const {
		return _p;
	}

	generic_ptr data() const {
		return _p;
	}

	constexpr size_t size() const {
		return _n;
	}

	void resize(size_t size = 0) {
		if (!size) {
			reset();
			return;
		}
		if (!_p) {
			return;
		}
		_n = size;
	}

	void reset() {
		if (!_p) {
			return;
		}
		_p = nullptr;
		_n = 0;
	}

	template <typename... Args>
	void reset(Args &&... args) {
		RUA_SASSERT((std::is_constructible<bytes_ref, Args...>::value));

		*this = bytes_ref(std::forward<Args>(args)...);
	}

private:
	generic_ptr _p;
	size_t _n;
};

template <typename Span>
inline bytes_view const_bytes_base<Span>::slice(
	ptrdiff_t begin_offset, ptrdiff_t end_offset_from_begin) const {
	assert(end_offset_from_begin > begin_offset);

	return bytes_view(
		_this()->data() + begin_offset,
		static_cast<size_t>(end_offset_from_begin - begin_offset));
}

template <typename Span>
inline bytes_view const_bytes_base<Span>::slice(ptrdiff_t begin_offset) const {
	return slice(begin_offset, _this()->size());
}

template <typename Span>
inline bytes_view const_bytes_base<Span>::
operator()(ptrdiff_t begin_offset, ptrdiff_t end_offset_from_begin) const {
	return slice(begin_offset, end_offset_from_begin);
}

template <typename Span>
inline bytes_view const_bytes_base<Span>::
operator()(ptrdiff_t begin_offset) const {
	return slice(begin_offset);
}

template <typename Span>
inline bytes_view bytes_base<Span>::slice(
	ptrdiff_t begin_offset, ptrdiff_t end_offset_from_begin) const {
	assert(end_offset_from_begin > begin_offset);

	return bytes_view(
		_this()->data() + begin_offset,
		static_cast<size_t>(end_offset_from_begin - begin_offset));
}

template <typename Span>
inline bytes_view bytes_base<Span>::slice(ptrdiff_t begin_offset) const {
	return slice(begin_offset, _this()->size());
}

template <typename Span>
inline bytes_ref bytes_base<Span>::slice(
	ptrdiff_t begin_offset, ptrdiff_t end_offset_from_begin) {
	assert(end_offset_from_begin > begin_offset);

	return bytes_ref(
		_this()->data() + begin_offset,
		static_cast<size_t>(end_offset_from_begin - begin_offset));
}

template <typename Span>
inline bytes_ref bytes_base<Span>::slice(ptrdiff_t begin_offset) {
	return slice(begin_offset, _this()->size());
}

template <typename Span>
inline bytes_view bytes_base<Span>::
operator()(ptrdiff_t begin_offset, ptrdiff_t end_offset_from_begin) const {
	return slice(begin_offset, end_offset_from_begin);
}

template <typename Span>
inline bytes_view bytes_base<Span>::operator()(ptrdiff_t begin_offset) const {
	return slice(begin_offset);
}

template <typename Span>
inline bytes_ref bytes_base<Span>::
operator()(ptrdiff_t begin_offset, ptrdiff_t end_offset_from_begin) {
	return slice(begin_offset, end_offset_from_begin);
}

template <typename Span>
inline bytes_ref bytes_base<Span>::operator()(ptrdiff_t begin_offset) {
	return slice(begin_offset);
}

template <typename Span>
template <typename... DestArgs>
inline size_t const_bytes_base<Span>::copy_to(DestArgs &&... dest) const {
	bytes_ref dest_ref(std::forward<DestArgs>(dest)...);
	auto cp_sz =
		_this()->size() < dest_ref.size() ? _this()->size() : dest_ref.size();
	if (!cp_sz) {
		return 0;
	}
	memcpy(dest_ref.data(), _this()->data(), cp_sz);
	return cp_sz;
}

template <typename Span>
template <typename... SrcArgs>
inline size_t bytes_base<Span>::copy_from(SrcArgs &&... src) const {
	return bytes_view(std::forward<SrcArgs>(src)...).copy_to(*_this());
}

inline bytes_ref as_bytes_ref(bytes_view src) {
	return bytes_ref(src.data().as<byte *>(), src.size());
}

class bytes : public bytes_ref {
public:
	constexpr bytes() = default;

	constexpr bytes(std::nullptr_t) : bytes() {}

	explicit bytes(size_t size) {
		if (!size) {
			return;
		}
		_alloc(size);
	}

	template <
		typename... Args,
		typename ArgsFront = decay_t<argments_front_t<Args...>>,
		typename = enable_if_t<
			(sizeof...(Args) > 1) ||
			(!std::is_base_of<bytes, ArgsFront>::value &&
			 !std::is_integral<ArgsFront>::value)>>
	bytes(Args &&... copy_src) {
		bytes_view v(std::forward<Args>(copy_src)...);
		if (!v.size()) {
			return;
		}
		_alloc(v.size());
		copy_from(v);
	}

	~bytes() {
		reset();
	}

	bytes(const bytes &src) : bytes(bytes_view(src)) {}

	bytes(bytes &&src) : bytes_ref(static_cast<bytes_ref &&>(std::move(src))) {
		if (src) {
			src.bytes_ref::reset();
		}
	}

	RUA_FORCE_INLINE bytes &operator=(const bytes &val) {
		reset(val);
		return *this;
	}

	RUA_FORCE_INLINE bytes &operator=(bytes &&val) {
		reset(std::move(val));
		return *this;
	}

	template <typename T>
	RUA_FORCE_INLINE enable_if_t<
		!std::is_base_of<bytes, decay_t<T>>::value &&
			std::is_convertible<T &&, bytes_view>::value,
		bytes &>
	operator=(T &&val) {
		reset(bytes_view(std::forward<T>(val)));
		return *this;
	}

	RUA_FORCE_INLINE bytes &operator+=(bytes_view tail) {
		resize(size() + tail.size());
		slice(size()).copy_from(tail);
		return *this;
	}

	size_t capacity() const {
		return data() ? _capacity() : 0;
	}

	void resize(size_t size = 0) {
		if (!data()) {
			reset(size);
			return;
		}
		if (_capacity() > size) {
			bytes_ref::resize(size);
			return;
		}
		bytes new_byts(size);
		new_byts.copy_from(*this);
		reset(std::move(new_byts));
	}

	void reset() {
		if (!data()) {
			return;
		}
		delete (data().as<char *>() - sizeof(size_t));
		bytes_ref::reset();
	}

	void reset(size_t size) {
		if (!size) {
			reset();
			return;
		}
		if (capacity() > size) {
			bytes_ref::resize(size);
			return;
		}
		reset();
		_alloc(size);
	}

	template <
		typename... Args,
		typename ArgsFront = argments_front_t<Args...>,
		typename DecayArgsFront = decay_t<ArgsFront>,
		typename = enable_if_t<
			(sizeof...(Args) > 1) ||
			(!(std::is_base_of<bytes, DecayArgsFront>::value &&
			   std::is_rvalue_reference<ArgsFront>::value) &&
			 !std::is_integral<DecayArgsFront>::value)>>
	void reset(Args &&... copy_src) {
		bytes_view v(std::forward<Args>(copy_src)...);
		if (!v.size()) {
			reset();
			return;
		}
		reset(v.size());
		copy_from(v);
	}

	void reset(bytes &&src) {
		if (!src.size()) {
			reset();
			src.reset();
			return;
		}
		if (data() && _capacity() >= src.size() &&
			_capacity() < src._capacity()) {
			bytes_ref::resize(src.size());
			copy_from(src);
			src.reset();
			return;
		}
		reset();
		bytes_ref::reset(src.data(), src.size());
		static_cast<bytes_ref &>(src).reset();
	}

private:
	RUA_FORCE_INLINE void _alloc(size_t size) {
		bytes_ref::reset(
			new char[sizeof(size_t) + size] + sizeof(size_t), size);
		bit_set<size_t>(data() - sizeof(size_t), size);
	}

	RUA_FORCE_INLINE size_t _capacity() const {
		return bit_get<size_t>(data() - sizeof(size_t));
	}
};

template <
	typename A,
	typename B,
	typename DecayA = decay_t<A>,
	typename DecayB = decay_t<B>>
RUA_FORCE_INLINE enable_if_t<
	(std::is_base_of<bytes_view, DecayA>::value ||
	 std::is_base_of<bytes_ref, DecayA>::value) &&
		(std::is_base_of<bytes_view, DecayB>::value ||
		 std::is_base_of<bytes_ref, DecayB>::value),
	bytes>
operator+(A &&a, B &&b) {
	bytes r(a.size() + b.size());
	r.copy_from(a);
	r.slice(a.size()).copy_from(b);
	return r;
}

template <
	typename A,
	typename B,
	typename DecayA = decay_t<A>,
	typename DecayB = decay_t<B>>
RUA_FORCE_INLINE enable_if_t<
	(std::is_base_of<bytes_view, DecayA>::value ||
	 std::is_base_of<bytes_ref, DecayA>::value) &&
		(!std::is_base_of<bytes_view, DecayB>::value &&
		 !std::is_base_of<bytes_ref, DecayB>::value),
	bytes>
operator+(A &&a, B &&b) {
	return a + bytes_view(std::forward<B>(b));
}

template <
	typename A,
	typename B,
	typename DecayA = decay_t<A>,
	typename DecayB = decay_t<B>>
RUA_FORCE_INLINE enable_if_t<
	(!std::is_base_of<bytes_view, DecayA>::value &&
	 !std::is_base_of<bytes_ref, DecayA>::value) &&
		(std::is_base_of<bytes_view, DecayB>::value ||
		 std::is_base_of<bytes_ref, DecayB>::value),
	bytes>
operator+(A &&a, B &&b) {
	return bytes_view(std::forward<A>(a)) + b;
}

template <typename Derived, size_t Size = nmax<size_t>()>
class bytes_block_base {
public:
	generic_ptr data() const {
		return this;
	}

	static constexpr size_t size() {
		return Size;
	}

protected:
	constexpr bytes_block_base() = default;
};

template <typename Derived, size_t Size = sizeof(Derived)>
class enable_bytes_accessor_from_this
	: private bytes_block_base<Derived, Size>,
	  public bytes_base<bytes_block_base<Derived, Size>> {
protected:
	constexpr enable_bytes_accessor_from_this() = default;
};

template <size_t Size, size_t Align = Size + Size % 2>
class bytes_block
	: public bytes_block_base<bytes_block<Size>, Size>,
	  public bytes_base<bytes_block_base<bytes_block<Size>, Size>> {
private:
	alignas(Align) char _raw[Size];
};

template <size_t Sz = nmax<size_t>()>
using bytes_block_ptr = bytes_block<Sz> *;

} // namespace rua

#endif
