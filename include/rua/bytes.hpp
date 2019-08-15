#ifndef _RUA_BYTES_HPP
#define _RUA_BYTES_HPP

#include "bit.hpp"
#include "byte.hpp"
#include "limits.hpp"
#include "macros.hpp"
#include "stldata.hpp"
#include "string/string_view.hpp"
#include "string/strlen.hpp"
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
	RUA_FORCE_INLINE const T &get_ref() const {
		return *reinterpret_cast<const T *>(_this()->data());
	}

	template <typename T>
	RUA_FORCE_INLINE const T &get_ref(ptrdiff_t offset) const {
		return *reinterpret_cast<const T *>(
			reinterpret_cast<uintptr_t>(_this()->data()) + offset);
	}

	template <typename T>
	RUA_FORCE_INLINE const T &get_ref_aligned(ptrdiff_t ix) const {
		return *reinterpret_cast<const T *>(_this()->data())[ix];
	}

	RUA_FORCE_INLINE const uint8_t &operator[](ptrdiff_t offset) const {
		return reinterpret_cast<const uint8_t *>(_this()->data())[offset];
	}

	inline bytes_view
	slice(ptrdiff_t begin_offset, ptrdiff_t end_offset_from_begin) const;

	inline bytes_view slice(ptrdiff_t begin_offset) const;

	inline bytes_view
	operator()(ptrdiff_t begin_offset, ptrdiff_t end_offset_from_begin) const;

	inline bytes_view operator()(ptrdiff_t begin_offset) const;

	inline size_t copy_to(bytes_ref dest) const;

	template <typename RelPtr, size_t SlotSize = sizeof(RelPtr)>
	const void *derel(ptrdiff_t pos = 0) const {
		return reinterpret_cast<void *>(
			reinterpret_cast<uintptr_t>(_this()->data()) + pos + SlotSize +
			get<RelPtr>(pos));
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
			return _n;
		}

		struct word {
			typename std::conditional<
				sizeof(CmpUnit) >= sizeof(size_t),
				CmpUnit,
				size_t>::type value;
			uint8_t size = sizeof(CmpUnit);

			RUA_FORCE_INLINE size_t eq(const uint8_t *begin) const {
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
					if (bit_get<uint8_t>(&value, 6) !=
						bit_get<uint8_t>(begin, 6)) {
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
					if (bit_get<uint8_t>(&value, 4) !=
						bit_get<uint8_t>(begin, 4)) {
						return 0;
					}
					RUA_FALLTHROUGH;

				case 4:
					return bit_get<uint32_t>(&value) ==
						   bit_get<uint32_t>(begin);

					////////////////////////////////////////////////////////////

				case 3:
					if (bit_get<uint8_t>(&value, 2) !=
						bit_get<uint8_t>(begin, 2)) {
						return 0;
					}
					RUA_FALLTHROUGH;

				case 2:
					return bit_get<uint16_t>(&value) ==
						   bit_get<uint16_t>(begin);

					////////////////////////////////////////////////////////////

				case 1:
					return bit_get<uint8_t>(&value) == bit_get<uint8_t>(begin);

					////////////////////////////////////////////////////////////

				default:
					for (size_t i = 0; i < size; ++i) {
						if (bit_get<uint8_t>(&value, i) !=
							bit_get<uint8_t>(begin, i)) {
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

		RUA_FORCE_INLINE void calc_hash(
			size_t &h, const uint8_t *begin, bool is_update = false) const {
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
				bit_set<uint8_t>(
					&_words[wi].value,
					last_sz,
					static_cast<uint8_t>(byt_vals.begin()[i]));
				++last_sz;

				_h += static_cast<size_t>(
					static_cast<uint8_t>(byt_vals.begin()[i]));
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

		match_pattern(std::initializer_list<std::initializer_list<uint8_t>>
						  byt_val_spls) {
			std::vector<uint16_t> byt_vals;

			auto sz = rua::nmax<size_t>();
			for (auto it = byt_val_spls.begin(); it != byt_val_spls.end();
				 ++it) {
				if (it->size() < sz) {
					sz = it->size();
				}
			}

			for (size_t i = 0; i < sz; ++i) {
				const uint8_t *b = nullptr;
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
			typename std::conditional<
				sizeof(CmpUnit) >= sizeof(size_t),
				CmpUnit,
				size_t>::type value = 0;
			uint8_t size = sizeof(CmpUnit);

			bool is_void() const {
				return !mask;
			}

			RUA_FORCE_INLINE size_t eq(const uint8_t *target) const {
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
					if (bit_get<uint8_t>(&value, 6) !=
						(bit_get<uint8_t>(target + 6) &
						 bit_get<uint8_t>(&mask, 6))) {
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
					if (bit_get<uint8_t>(&value, 4) !=
						(bit_get<uint8_t>(target + 4) &
						 bit_get<uint8_t>(&mask, 4))) {
						return 0;
					}
					RUA_FALLTHROUGH;

				case 4:
					return bit_get<uint32_t>(&value) ==
						   (bit_get<uint32_t>(target) &
							bit_get<uint32_t>(&mask));

					////////////////////////////////////////////////////////////

				case 3:
					if (bit_get<uint8_t>(&value, 2) !=
						(bit_get<uint8_t>(target + 2) &
						 bit_get<uint8_t>(&mask, 2))) {
						return 0;
					}
					RUA_FALLTHROUGH;

				case 2:
					return bit_get<uint16_t>(&value) ==
						   (bit_get<uint16_t>(target) &
							bit_get<uint16_t>(&mask));

					////////////////////////////////////////////////////////////

				case 1:
					return bit_get<uint8_t>(&value) ==
						   (bit_get<uint8_t>(target) & bit_get<uint8_t>(&mask));

					////////////////////////////////////////////////////////////

				default:
					for (size_t i = 0; i < size; ++i) {
						if (bit_get<uint8_t>(&value, i) !=
							(bit_get<uint8_t>(target + i) &
							 bit_get<uint8_t>(&mask, i))) {
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
		calc_hash(size_t &, const uint8_t *, bool = false) {}

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
					bit_set<uint8_t>(&_words.back().mask, last_sz, 255);
					bit_set<uint8_t>(
						&_words.back().value,
						last_sz,
						static_cast<uint8_t>(byt_vals.begin()[i]));
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
			return search_result{npos};
		}

		auto begin = &(*_this())[0];
		const uint8_t *end = begin + _this()->size() - byts.size();

		size_t h = 0;
		const uint8_t *cmp_ptr = nullptr;

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
	RUA_FORCE_INLINE const T &get_ref() const {
		return *reinterpret_cast<const T *>(_this()->data());
	}

	template <typename T>
	RUA_FORCE_INLINE T &get_ref() {
		return *reinterpret_cast<T *>(_this()->data());
	}

	template <typename T>
	RUA_FORCE_INLINE const T &get_ref(ptrdiff_t offset) const {
		return *reinterpret_cast<const T *>(
			reinterpret_cast<uintptr_t>(_this()->data()) + offset);
	}

	template <typename T>
	RUA_FORCE_INLINE T &get_ref(ptrdiff_t offset) {
		return *reinterpret_cast<T *>(
			reinterpret_cast<uintptr_t>(_this()->data()) + offset);
	}

	template <typename T>
	RUA_FORCE_INLINE const T &get_ref_aligned(ptrdiff_t ix) const {
		return *reinterpret_cast<const T *>(_this()->data())[ix];
	}

	template <typename T>
	RUA_FORCE_INLINE T &get_ref_aligned(ptrdiff_t ix) {
		return *reinterpret_cast<T *>(_this()->data())[ix];
	}

	RUA_FORCE_INLINE const uint8_t &operator[](ptrdiff_t offset) const {
		return reinterpret_cast<const uint8_t *>(_this()->data())[offset];
	}

	RUA_FORCE_INLINE uint8_t &operator[](ptrdiff_t offset) {
		return reinterpret_cast<uint8_t *>(_this()->data())[offset];
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

	inline size_t copy_from(bytes_view src);

	template <typename RelPtr, size_t SlotSize = sizeof(RelPtr)>
	void *derel(ptrdiff_t pos = 0) {
		return reinterpret_cast<void *>(
			reinterpret_cast<uintptr_t>(_this()->data()) + pos + SlotSize +
			this->template get<RelPtr>(pos));
	}

	template <typename RelPtr, size_t SlotSize = sizeof(RelPtr)>
	const void *derel(ptrdiff_t pos = 0) const {
		return reinterpret_cast<void *>(
			reinterpret_cast<uintptr_t>(_this()->data()) + pos + SlotSize +
			this->template get<RelPtr>(pos));
	}

	template <
		typename RelPtr,
		size_t SlotSize = sizeof(RelPtr),
		typename AbsPoint>
	RelPtr enrel(AbsPoint *abs_ptr, ptrdiff_t pos = 0) {
		auto rel_ptr = static_cast<RelPtr>(
			abs_ptr -
			(reinterpret_cast<uintptr_t>(_this()->data()) + pos + SlotSize));
		this->template set<RelPtr>(pos, rel_ptr);
		return rel_ptr;
	}

	template <typename T = uint8_t>
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

	constexpr bytes_view(const byte *ptr, size_t size) :
		_p(ptr),
		_n(ptr ? size : 0) {}

	template <
		typename T,
		typename = typename std::enable_if<
			!std::is_same<T, byte>::value &&
			!std::is_convertible<const T *, string_view>::value &&
			!std::is_convertible<const T *, wstring_view>::value &&
			!std::is_function<T>::value>::type>
	constexpr bytes_view(const T *ptr, size_t size = sizeof(T)) :
		bytes_view(reinterpret_cast<const byte *>(ptr), size) {}

	template <
		typename T,
		typename CArray = typename std::remove_reference<T>::type,
		typename = typename std::enable_if<std::is_array<CArray>::value>::type>
	constexpr bytes_view(T &&c_array_ref) :
		bytes_view(&c_array_ref, sizeof(CArray)) {}

	template <typename R, typename... A>
	constexpr bytes_view(R (*ptr)(A...), size_t size = nmax<size_t>()) :
		_p(reinterpret_cast<const byte *>(ptr)),
		_n(size) {}

	bytes_view(const char *c_str, size_t size) :
		_p(reinterpret_cast<const byte *>(c_str)),
		_n(size) {}

	bytes_view(const char *c_str) :
		_p(reinterpret_cast<const byte *>(c_str)),
		_n(c_str ? strlen(c_str) : 0) {}

	bytes_view(const wchar_t *c_wstr, size_t size) :
		_p(reinterpret_cast<const byte *>(c_wstr)),
		_n(size) {}

	bytes_view(const wchar_t *c_wstr) :
		_p(reinterpret_cast<const byte *>(c_wstr)),
		_n(strlen(c_wstr) * sizeof(wchar_t)) {}

	template <
		typename T,
		typename = typename std::enable_if<
			!std::is_convertible<T &, string_view>::value &&
			!std::is_convertible<T &, wstring_view>::value &&
			!std::is_base_of<bytes_view, typename std::decay<T>::type>::value &&
			!is_span<T>::value &&
			!std::is_member_function_pointer<T>::value>::type>
	constexpr bytes_view(const T &const_val_ref, size_t size = sizeof(T)) :
		bytes_view(&const_val_ref, size) {}

	template <
		typename T,
		typename = typename std::enable_if<
			!std::is_convertible<T &, string_view>::value &&
			!std::is_convertible<T &, wstring_view>::value &&
			!std::is_base_of<bytes_view, typename std::decay<T>::type>::value &&
			!is_span<T>::value>::type,
		typename = typename std::enable_if<
			std::is_member_function_pointer<T>::value>::type>
	constexpr bytes_view(const T &mbr_fn_ref, size_t size = nmax<size_t>()) :
		bytes_view(reinterpret_cast<const void *>(mbr_fn_ref), size) {}

	template <
		typename T,
		typename = typename std::enable_if<
			!std::is_base_of<bytes_view, typename std::decay<T>::type>::value>::
			type,
		typename SpanElem = get_span_element_t<const T>>
	bytes_view(const T &span) :
		bytes_view(span.data(), span.size() * sizeof(SpanElem)) {}

	operator bool() const {
		return _p;
	}

	const byte *data() const {
		return _p;
	}

	size_t size() const {
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
	const byte *_p;
	size_t _n;
};

class bytes_ref : public bytes_base<bytes_ref> {
public:
	constexpr bytes_ref() : _p(nullptr), _n(0) {}

	constexpr bytes_ref(std::nullptr_t) : bytes_ref() {}

	constexpr bytes_ref(byte *ptr, size_t size) : _p(ptr), _n(ptr ? size : 0) {}

	template <
		typename T,
		typename = typename std::enable_if<
			!std::is_const<T>::value && !std::is_same<T, byte>::value &&
			!std::is_convertible<T *, string_view>::value &&
			!std::is_convertible<T *, wstring_view>::value &&
			!std::is_function<T>::value>::type>
	constexpr bytes_ref(T *ptr, size_t size = sizeof(T)) :
		bytes_ref(reinterpret_cast<byte *>(ptr), size) {}

	template <
		typename T,
		typename CArray = typename std::remove_reference<T>::type,
		typename = typename std::enable_if<
			std::is_array<CArray>::value &&
			!std::is_const<typename std::remove_pointer<
				typename std::decay<CArray>::type>::type>::value>::type>
	constexpr bytes_ref(T &&c_array_ref) :
		bytes_ref(&c_array_ref, sizeof(CArray)) {}

	bytes_ref(char *c_str, size_t size) :
		_p(reinterpret_cast<byte *>(c_str)),
		_n(size) {}

	bytes_ref(char *c_str) :
		_p(reinterpret_cast<byte *>(c_str)),
		_n(c_str ? strlen(c_str) : 0) {}

	bytes_ref(wchar_t *c_wstr, size_t size) :
		_p(reinterpret_cast<byte *>(c_wstr)),
		_n(size) {}

	bytes_ref(wchar_t *c_wstr) :
		_p(reinterpret_cast<byte *>(c_wstr)),
		_n(strlen(c_wstr) * sizeof(wchar_t)) {}

	template <
		typename T,
		typename = typename std::enable_if<
			!std::is_base_of<bytes_ref, typename std::decay<T>::type>::value>::
			type,
		typename SpanElem = get_span_element_t<T>,
		typename =
			typename std::enable_if<!std::is_const<SpanElem>::value>::type>
	bytes_ref(T &span) :
		bytes_ref(stldata(span), span.size() * sizeof(SpanElem)) {}

	operator bool() const {
		return _p;
	}

	const byte *data() const {
		return _p;
	}

	byte *data() {
		return _p;
	}

	size_t size() const {
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

		*this = bytes_ref(std::forward<Args>(args)...);
	}

private:
	byte *_p;
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
inline size_t const_bytes_base<Span>::copy_to(bytes_ref dest) const {
	auto cp_sz = _this()->size() < dest.size() ? _this()->size() : dest.size();
	if (!cp_sz) {
		return 0;
	}
	memcpy(dest.data(), _this()->data(), cp_sz);
	return cp_sz;
}

template <typename Span>
inline size_t bytes_base<Span>::copy_from(bytes_view src) {
	return src.copy_to(*_this());
}

inline bytes_ref as_bytes_ref(bytes_view src) {
	return bytes_ref(const_cast<byte *>(src.data()), src.size());
}

class bytes : public bytes_ref {
public:
	constexpr bytes() = default;

	constexpr bytes(std::nullptr_t) : bytes() {}

	bytes(size_t size) : bytes_ref(size ? new char[size] : nullptr, size) {}

	template <
		typename... Args,
		typename ArgsFront =
			typename std::decay<argments_front_t<Args...>>::type,
		typename = typename std::enable_if<
			!std::is_base_of<bytes, ArgsFront>::value &&
			!std::is_integral<ArgsFront>::value>::type>
	bytes(Args &&... copy_src) {
		bytes_view v(std::forward<Args>(copy_src)...);
		if (!v.size()) {
			return;
		}
		bytes_ref::reset(new char[v.size()], v.size());
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

	RUA_OVERLOAD_ASSIGNMENT(bytes)

	inline bytes_view
	slice(ptrdiff_t begin_offset, ptrdiff_t end_offset_from_begin) const & {
		return bytes_ref::slice(begin_offset, end_offset_from_begin);
	}

	inline bytes_view slice(ptrdiff_t begin_offset) const & {
		return slice(begin_offset, size());
	}

	inline bytes_ref
	slice(ptrdiff_t begin_offset, ptrdiff_t end_offset_from_begin) & {
		return bytes_ref::slice(begin_offset, end_offset_from_begin);
	}

	inline bytes_ref slice(ptrdiff_t begin_offset) & {
		return slice(begin_offset, size());
	}

	inline bytes
	slice(ptrdiff_t begin_offset, ptrdiff_t end_offset_from_begin) && {
		bytes r(slice(begin_offset, end_offset_from_begin));
		reset();
		return r;
	}

	inline bytes slice(ptrdiff_t begin_offset) && {
		return std::move(*this).slice(begin_offset, size());
	}

	inline bytes_view operator()(
		ptrdiff_t begin_offset, ptrdiff_t end_offset_from_begin) const & {
		return slice(begin_offset, end_offset_from_begin);
	}

	inline bytes_view operator()(ptrdiff_t begin_offset) const & {
		return slice(begin_offset);
	}

	inline bytes_ref
	operator()(ptrdiff_t begin_offset, ptrdiff_t end_offset_from_begin) & {
		return slice(begin_offset, end_offset_from_begin);
	}

	inline bytes_ref operator()(ptrdiff_t begin_offset) & {
		return slice(begin_offset);
	}

	inline bytes
	operator()(ptrdiff_t begin_offset, ptrdiff_t end_offset_from_begin) && {
		return std::move(*this).slice(begin_offset, end_offset_from_begin);
	}

	inline bytes operator()(ptrdiff_t begin_offset) && {
		return std::move(*this).slice(begin_offset);
	}

	void resize(size_t size = 0) {
		if (!size) {
			reset();
			return;
		}
		if (!data()) {
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
		delete data();
		bytes_ref::reset();
	}

	template <typename... Args>
	void reset(Args &&... args) {
		RUA_SASSERT((std::is_constructible<bytes_view, Args...>::value));

		*this = bytes(std::forward<Args>(args)...);
	}
};

template <typename Derived, size_t Size = nmax<size_t>()>
class bytes_block_base {
public:
	void *data() {
		return static_cast<Derived *>(this);
	}

	const void *data() const {
		return static_cast<const Derived *>(this);
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

template <size_t Size>
class bytes_block
	: public bytes_block_base<bytes_block<Size>, Size>,
	  public bytes_base<bytes_block_base<bytes_block<Size>, Size>> {
private:
	uint8_t _raw[Size];
};

template <size_t Sz = nmax<size_t>()>
using bytes_block_ptr = bytes_block<Sz> *;

} // namespace rua

#endif
