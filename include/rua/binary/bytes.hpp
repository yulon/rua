#ifndef _rua_binary_bytes_hpp
#define _rua_binary_bytes_hpp

/*
	Q: Why don't I use std::byte?
	A: Because I suspect std::byte is used to make Microsoft feel sick.

	1. All operators are implemented by operator overloading,
	   but MSVC does not inline code in debug mode,
	   which causes programs in debug mode to run slowly.

	2. List initialization declarations are very redundant,
	   such as {std::byte{n}, ...} or {static_cast<std::byte>(n), ...},
	   but G++ can use {{n}, ...}.
*/

#include "bits.hpp"

#include "../optional.hpp"
#include "../range.hpp"
#include "../span.hpp"
#include "../string/conv.hpp"
#include "../string/len.hpp"
#include "../string/view.hpp"
#include "../util.hpp"

#include <cassert>
#include <cstring>
#include <string>
#include <vector>

namespace rua {

class bytes_view;
class bytes_ref;
class bytes;
class bytes_pattern;

template <typename Bytes>
class basic_bytes_finder;

using const_bytes_finder = basic_bytes_finder<bytes_view>;
using bytes_finder = basic_bytes_finder<bytes_ref>;

using const_bytes_rfinder = reverse_iterator<const_bytes_finder>;
using bytes_rfinder = reverse_iterator<bytes_finder>;

template <typename Span>
class const_bytes_base : private disable_as_to_string {
public:
	generic_ptr data_generic() const {
		return $this()->data();
	}

	constexpr explicit operator bool() const {
		return $this()->size();
	}

	const uchar *begin() const {
		return $this()->data();
	}

	const uchar *end() const {
		return $this()->data() + $this()->size();
	}

	template <typename T>
	T get() const {
		return bit_get<T>($this()->data());
	}

	template <typename T>
	T get(ptrdiff_t offset) const {
		return bit_get<T>($this()->data(), offset);
	}

	template <typename T>
	T aligned_get(ptrdiff_t ix) const {
		return bit_aligned_get<T>($this()->data(), ix);
	}

	template <typename T>
	const T &as() const {
		return bit_as<const T>($this()->data());
	}

	template <typename T>
	const T &as(ptrdiff_t offset) const {
		return bit_as<const T>($this()->data(), offset);
	}

	template <typename T>
	const T &aligned_as(ptrdiff_t ix) const {
		return bit_aligned_as<const T>($this()->data(), ix);
	}

	const uchar &operator[](ptrdiff_t offset) const {
		return $this()->data()[offset];
	}

	inline bytes_view slice(ptrdiff_t begin_offset, ptrdiff_t end_offset) const;

	inline bytes_view slice(ptrdiff_t begin_offset) const;

	inline bytes_view
	operator()(ptrdiff_t begin_offset, ptrdiff_t end_offset) const;

	inline bytes_view operator()(ptrdiff_t begin_offset) const;

	template <typename RelPtr, size_t SlotSize = sizeof(RelPtr)>
	generic_ptr derel(ptrdiff_t pos = 0) const {
		return $this()->data() + pos + SlotSize + get<RelPtr>(pos);
	}

	inline bytes reverse() const;

	template <size_t Unit>
	inline bytes reverse() const;

	inline bool equal(bytes_view) const;

	inline bool operator==(bytes_view) const;

	inline optional<size_t>
	index_of(const bytes_pattern &, size_t start_pos = 0) const;

	inline const_bytes_finder find(bytes_pattern, size_t start_pos = 0) const;

	inline optional<size_t>
	last_index_of(const bytes_pattern &, size_t start_pos = nullpos) const;

	inline const_bytes_rfinder
	rfind(bytes_pattern, size_t start_pos = nullpos) const;

protected:
	const_bytes_base() = default;

private:
	const Span *$this() const {
		return static_cast<const Span *>(this);
	}
};

template <typename Span>
class bytes_base : public const_bytes_base<Span> {
public:
	uchar *begin() {
		return $this()->data();
	}

	const uchar *begin() const {
		return $this()->data();
	}

	uchar *end() {
		return $this()->data() + $this()->size();
	}

	const uchar *end() const {
		return $this()->data() + $this()->size();
	}

	template <typename T>
	void set(const T &val) {
		return bit_set<T>($this()->data(), val);
	}

	template <typename T>
	void set(const T &val, ptrdiff_t offset) {
		return bit_set<T>($this()->data(), offset, val);
	}

	template <typename T>
	void aligned_set(const T &val, ptrdiff_t ix) {
		return bit_aligned_set<T>($this()->data(), val, ix);
	}

	template <typename T>
	const T &as() const {
		return bit_as<const T>($this()->data());
	}

	template <typename T>
	T &as() {
		return bit_as<T>($this()->data());
	}

	template <typename T>
	const T &as(ptrdiff_t offset) const {
		return bit_as<const T>($this()->data(), offset);
	}

	template <typename T>
	T &as(ptrdiff_t offset) {
		return bit_as<T>($this()->data(), offset);
	}

	template <typename T>
	const T &aligned_as(ptrdiff_t ix) const {
		return bit_aligned_as<const T>($this()->data(), ix);
	}

	template <typename T>
	T &aligned_as(ptrdiff_t ix) {
		return bit_aligned_as<T>($this()->data(), ix);
	}

	const uchar &operator[](ptrdiff_t offset) const {
		return $this()->data()[offset];
	}

	uchar &operator[](ptrdiff_t offset) {
		return $this()->data()[offset];
	}

	inline bytes_view slice(ptrdiff_t begin_offset, ptrdiff_t end_offset) const;

	inline bytes_view slice(ptrdiff_t begin_offset) const;

	inline bytes_ref slice(ptrdiff_t begin_offset, ptrdiff_t end_offset);

	inline bytes_ref slice(ptrdiff_t begin_offset);

	inline bytes_view
	operator()(ptrdiff_t begin_offset, ptrdiff_t end_offset) const;

	inline bytes_view operator()(ptrdiff_t begin_offset) const;

	inline bytes_ref operator()(ptrdiff_t begin_offset, ptrdiff_t end_offset);

	inline bytes_ref operator()(ptrdiff_t begin_offset);

	template <typename... SrcArgs>
	inline size_t copy(SrcArgs &&...src);

	template <typename RelPtr, size_t SlotSize = sizeof(RelPtr)>
	RelPtr enrel(generic_ptr abs_ptr, ptrdiff_t pos = 0) {
		auto rel_ptr =
			static_cast<RelPtr>(abs_ptr - ($this()->data() + pos + SlotSize));
		this->template set<RelPtr>(pos, rel_ptr);
		return rel_ptr;
	}

	inline bytes_finder find(bytes_pattern, size_t start_pos = 0);

	inline const_bytes_finder find(bytes_pattern, size_t start_pos = 0) const;

	inline bytes_rfinder rfind(bytes_pattern, size_t start_pos = nullpos);

	inline const_bytes_rfinder
	rfind(bytes_pattern, size_t start_pos = nullpos) const;

private:
	Span *$this() {
		return static_cast<Span *>(this);
	}

	const Span *$this() const {
		return static_cast<const Span *>(this);
	}
};

class bytes_view : public const_bytes_base<bytes_view> {
public:
	constexpr bytes_view(std::nullptr_t = nullptr) : $p(nullptr), $n(0) {}

	constexpr bytes_view(const uchar *ptr, size_t size) :
		$p(ptr), $n(ptr ? size : 0) {}

	RUA_CONSTEXPR_14 bytes_view(std::initializer_list<uchar> il) :
		bytes_view(il.begin(), il.size()) {}

	template <
		typename Span,
		typename SpanTraits = span_traits<Span &&>,
		typename = enable_if_t<
			!std::is_base_of<bytes_view, decay_t<Span>>::value &&
			std::is_same<typename SpanTraits::value_type, uchar>::value>>
	constexpr bytes_view(Span &&span) :
		bytes_view(
			rua::data(std::forward<Span>(span)),
			rua::size(std::forward<Span>(span))) {}

	~bytes_view() {
		reset();
	}

	bytes_view(const bytes_view &src) : $p(src.$p), $n(src.$n) {}

	bytes_view(bytes_view &&src) : $p(src.$p), $n(src.$n) {
		src.reset();
	}

	RUA_OVERLOAD_ASSIGNMENT(bytes_view)

	const uchar *data() const {
		return $p;
	}

	constexpr size_t size() const {
		return $n;
	}

	void resize(size_t size) {
		if (!size) {
			reset();
			return;
		}
		if (!$p) {
			return;
		}
		$n = size;
	}

	void reset() {
		if (!$p) {
			return;
		}
		$p = nullptr;
		$n = 0;
	}

	template <typename... Args>
	void reset(Args &&...args) {
		RUA_SASSERT((std::is_constructible<bytes_view, Args...>::value));

		*this = bytes_view(std::forward<Args>(args)...);
	}

private:
	const uchar *$p;
	size_t $n;
};

template <typename Span>
inline bytes_view const_bytes_base<Span>::slice(
	ptrdiff_t begin_offset, ptrdiff_t end_offset) const {
	assert(end_offset >= begin_offset);

	if (begin_offset == end_offset) {
		return nullptr;
	}
	return bytes_view(
		$this()->data() + begin_offset,
		static_cast<size_t>(end_offset - begin_offset));
}

template <typename Span>
inline bytes_view const_bytes_base<Span>::slice(ptrdiff_t begin_offset) const {
	return slice(begin_offset, $this()->size());
}

template <typename Span>
inline bytes_view const_bytes_base<Span>::operator()(
	ptrdiff_t begin_offset, ptrdiff_t end_offset) const {
	return slice(begin_offset, end_offset);
}

template <typename Span>
inline bytes_view
const_bytes_base<Span>::operator()(ptrdiff_t begin_offset) const {
	return slice(begin_offset);
}

template <typename Span>
inline bool const_bytes_base<Span>::equal(bytes_view target) const {
	auto sz = $this()->size();
	if (sz != target.size()) {
		return false;
	}
	auto a = $this()->data();
	auto b = target.data();
	if (a == b) {
		return true;
	}
	if (!a || !b) {
		return false;
	}
	return bit_equal(a, b, sz);
}

template <typename Span>
inline bool const_bytes_base<Span>::operator==(bytes_view target) const {
	return equal(target);
}

class bytes_ref : public bytes_base<bytes_ref> {
public:
	constexpr bytes_ref(std::nullptr_t = nullptr) : $p(nullptr), $n(0) {}

	constexpr bytes_ref(uchar *ptr, size_t size) :
		$p(ptr), $n(ptr ? size : 0) {}

	template <
		typename Span,
		typename SpanTraits = span_traits<Span &&>,
		typename = enable_if_t<
			!std::is_base_of<bytes_ref, decay_t<Span>>::value &&
			std::is_same<typename SpanTraits::value_type, uchar>::value &&
			!std::is_const<typename SpanTraits::element_type>::value>>
	constexpr bytes_ref(Span &&span) :
		bytes_ref(
			rua::data(std::forward<Span>(span)),
			rua::size(std::forward<Span>(span))) {}

	~bytes_ref() {
		reset();
	}

	bytes_ref(const bytes_ref &src) : $p(src.$p), $n(src.$n) {}

	bytes_ref(bytes_ref &&src) : $p(src.$p), $n(src.$n) {
		src.reset();
	}

	RUA_OVERLOAD_ASSIGNMENT(bytes_ref)

	uchar *data() {
		return $p;
	}

	const uchar *data() const {
		return $p;
	}

	constexpr size_t size() const {
		return $n;
	}

	void resize(size_t size) {
		if (!size) {
			reset();
			return;
		}
		if (!$p) {
			return;
		}
		$n = size;
	}

	void reset() {
		if (!$p) {
			return;
		}
		$p = nullptr;
		$n = 0;
	}

	template <typename... Args>
	void reset(Args &&...args) {
		RUA_SASSERT((std::is_constructible<bytes_ref, Args...>::value));

		*this = bytes_ref(std::forward<Args>(args)...);
	}

private:
	uchar *$p;
	size_t $n;
};

template <typename Span>
inline bytes_view
bytes_base<Span>::slice(ptrdiff_t begin_offset, ptrdiff_t end_offset) const {
	assert(end_offset >= begin_offset);

	if (begin_offset == end_offset) {
		return nullptr;
	}
	return bytes_view(
		$this()->data() + begin_offset,
		static_cast<size_t>(end_offset - begin_offset));
}

template <typename Span>
inline bytes_view bytes_base<Span>::slice(ptrdiff_t begin_offset) const {
	return slice(begin_offset, $this()->size());
}

template <typename Span>
inline bytes_ref
bytes_base<Span>::slice(ptrdiff_t begin_offset, ptrdiff_t end_offset) {
	assert(end_offset >= begin_offset);

	if (begin_offset == end_offset) {
		return nullptr;
	}
	return bytes_ref(
		$this()->data() + begin_offset,
		static_cast<size_t>(end_offset - begin_offset));
}

template <typename Span>
inline bytes_ref bytes_base<Span>::slice(ptrdiff_t begin_offset) {
	return slice(begin_offset, $this()->size());
}

template <typename Span>
inline bytes_view bytes_base<Span>::operator()(
	ptrdiff_t begin_offset, ptrdiff_t end_offset) const {
	return slice(begin_offset, end_offset);
}

template <typename Span>
inline bytes_view bytes_base<Span>::operator()(ptrdiff_t begin_offset) const {
	return slice(begin_offset);
}

template <typename Span>
inline bytes_ref
bytes_base<Span>::operator()(ptrdiff_t begin_offset, ptrdiff_t end_offset) {
	return slice(begin_offset, end_offset);
}

template <typename Span>
inline bytes_ref bytes_base<Span>::operator()(ptrdiff_t begin_offset) {
	return slice(begin_offset);
}

template <typename Span>
template <typename... SrcArgs>
inline size_t bytes_base<Span>::copy(SrcArgs &&...src) {
	bytes_view src_b(std::forward<SrcArgs>(src)...);
	auto cp_sz =
		src_b.size() < $this()->size() ? src_b.size() : $this()->size();
	if (!cp_sz) {
		return 0;
	}
	memcpy($this()->data(), src_b.data(), cp_sz);
	return cp_sz;
}

template <
	typename Ptr,
	typename PurePtr = remove_cvref_t<Ptr>,
	typename E = remove_pointer_t<remove_cv_t<PurePtr>>,
	bool IsConst = std::is_const<E>::value>
inline enable_if_t<
	std::is_pointer<PurePtr>::value && (size_of<E>::value > 0),
	conditional_t<IsConst, bytes_view, bytes_ref>>
as_bytes(Ptr &&ptr) {
	return {
		reinterpret_cast<conditional_t<IsConst, const uchar, uchar> *>(ptr),
		sizeof(E)};
}

template <typename E, bool IsConst = std::is_const<E>::value>
inline conditional_t<IsConst, bytes_view, bytes_ref>
as_bytes(E *ptr, size_t size) {
	return {
		reinterpret_cast<conditional_t<IsConst, const uchar, uchar> *>(ptr),
		size};
}

template <typename T, typename R, typename... Args>
inline bytes_view as_bytes(R (T::*mem_fn_ptr)(Args...), size_t size) {
	return {bit_cast<const uchar *>(mem_fn_ptr), size};
}

template <
	typename T,
	bool IsConst = std::is_const<remove_reference_t<T>>::value>
inline enable_if_t<
	!std::is_convertible<T &&, generic_ptr>::value && !is_span<T &&>::value,
	conditional_t<IsConst, bytes_view, bytes_ref>>
as_bytes(T &&ref, size_t size = sizeof(T)) {
	return {
		reinterpret_cast<conditional_t<IsConst, const uchar, uchar> *>(&ref),
		size};
}

template <
	typename Span,
	typename SpanTraits = span_traits<Span &&>,
	bool IsConst = std::is_const<typename SpanTraits::element_type>::value>
inline enable_if_t<
	!std::is_base_of<bytes_view, decay_t<Span>>::value &&
		(!std::is_array<remove_reference_t<Span>>::value ||
		 (!std::is_same<typename SpanTraits::value_type, char>::value &&
		  !std::is_same<typename SpanTraits::value_type, wchar_t>::value)),
	conditional_t<IsConst, bytes_view, bytes_ref>>
as_bytes(Span &&span) {
	return {
		reinterpret_cast<conditional_t<IsConst, const uchar, uchar> *>(
			rua::data(std::forward<Span>(span))),
		rua::size(std::forward<Span>(span)) *
			sizeof(typename SpanTraits::element_type)};
}

inline bytes_view as_bytes(const char *c_str) {
	return as_bytes(string_view(c_str));
}

inline bytes_ref as_bytes(char *c_str) {
	return {reinterpret_cast<uchar *>(c_str), c_str_len(c_str)};
}

inline bytes_view as_bytes(const wchar_t *c_str) {
	return as_bytes(wstring_view(c_str));
}

inline bytes_ref as_bytes(wchar_t *c_str) {
	return {
		reinterpret_cast<uchar *>(c_str), c_str_len(c_str) * sizeof(wchar_t)};
}

template <typename T>
inline bytes_ref as_writable_bytes(T &&data) {
	auto b = as_bytes(data);
	return bytes_ref(const_cast<uchar *>(b.data()), b.size());
}

template <typename Bytes>
inline string_view as_string(const const_bytes_base<Bytes> &b) {
	return static_cast<const Bytes &>(b).size()
			   ? string_view(
					 reinterpret_cast<const char *>(
						 static_cast<const Bytes &>(b).data()),
					 static_cast<const Bytes &>(b).size())
			   : "";
}

class bytes : public bytes_ref {
public:
	constexpr bytes(std::nullptr_t = nullptr) : bytes_ref() {}

	explicit bytes(size_t size) {
		if (!size) {
			return;
		}
		$alloc(size);
	}

	RUA_CONSTRUCTIBLE_CONCEPT(Args, bytes_view, bytes)
	bytes(Args &&...copy_src) {
		bytes_view bv(std::forward<Args>(copy_src)...);
		if (!bv.size()) {
			return;
		}
		$alloc(bv.size());
		copy(bv);
	}

	bytes(std::initializer_list<uchar> il) : bytes(il.begin(), il.size()) {}

	~bytes() {
		reset();
	}

	bytes(const bytes &src) : bytes(bytes_view(src)) {}

	bytes(bytes &&src) : bytes_ref(static_cast<bytes_ref &&>(std::move(src))) {}

	RUA_OVERLOAD_ASSIGNMENT_S(bytes)

	bytes &operator+=(bytes_view tail) {
		auto old_sz = size();
		auto new_sz = old_sz + tail.size();

		resize(new_sz);
		assert(size() == new_sz);

		slice(old_sz).copy(tail);

		return *this;
	}

	void resize(size_t size) {
		if (!data()) {
			reset(size);
			return;
		}
		if ($capacity() > size) {
			bytes_ref::resize(size);
			return;
		}
		bytes new_byts(size);
		new_byts.copy(*this);
		*this = std::move(new_byts);
	}

	size_t capacity() const {
		return data() ? $capacity() : 0;
	}

	void reserve(size_t cap) {
		auto sz = size();
		resize(cap);
		resize(sz);
	}

	void reset() {
		if (!data()) {
			return;
		}
		delete (data() - sizeof(size_t));
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
		$alloc(size);
	}

private:
	void $alloc(size_t size) {
		bytes_ref::reset(
			new uchar[sizeof(size_t) + size] + sizeof(size_t), size);
		bit_set<size_t>(data() - sizeof(size_t), size);
	}

	size_t $capacity() const {
		return bit_get<size_t>(data() - sizeof(size_t));
	}
};

template <typename Span>
inline bytes const_bytes_base<Span>::reverse() const {
	auto n = $this()->size();
	bytes r(n);
	auto p = $this()->data();
	auto r_p = r.data();
	for (size_t i = 0; i < n; ++i) {
		r_p[i] = p[n - 1 - i];
	}
	return r;
}

template <typename Span>
template <size_t Unit>
inline bytes const_bytes_base<Span>::reverse() const {
	auto n = $this()->size();
	bytes r(n);
	auto p = $this()->data();
	auto r_p = r.data();
	for (size_t i = 0; i < n; i += Unit) {
		memcpy(&r_p[i], &p[n - Unit - i], Unit);
	}
	return r;
}

inline bytes operator+(bytes_view a, bytes_view b) {
	bytes r(a.size() + b.size());
	r.copy(a);
	r.slice(a.size()).copy(b);
	return r;
}

template <typename T>
inline bytes to_bytes(T &&data) {
	return as_bytes(data);
}

class bytes_pattern {
public:
	bytes_pattern() = default;

	RUA_CONSTRUCTIBLE_CONCEPT(Args, bytes, bytes_pattern)
	bytes_pattern(Args &&...bytes_args) :
		$mb(std::forward<Args>(bytes_args)...) {}

	bytes_pattern(rua::bytes masked, rua::bytes mask) :
		$mb(std::move(masked)), _m(std::move(mask)) {}

	template <
		typename IntList,
		typename IntListTraits = range_traits<IntList>,
		typename Int = typename IntListTraits::value_type,
		typename = enable_if_t<
			std::is_integral<Int>::value && (sizeof(Int) > sizeof(uchar))>>
	bytes_pattern(IntList &&li) {
		$input(li);
	}

	bytes_pattern(std::initializer_list<uint16_t> il) {
		$input(il);
	}

	size_t size() const {
		return $mb.size();
	}

	bytes_view masked() const {
		return $mb;
	}

	bytes_view mask() const {
		return _m;
	}

	bool contains(bytes_view byts) const {
		auto sz = byts.size();
		if (sz != size()) {
			return false;
		}
		if (!_m) {
			return bit_equal($mb.data(), byts.data(), sz);
		}
		return bit_contains($mb.data(), byts.data(), _m.data(), sz);
	}

private:
	bytes $mb, _m;

	template <typename IntList>
	void $input(IntList &&li) {
		$mb.reset(li.size());
		auto begin = li.begin();
		size_t i;
		for (i = 0; i < $mb.size(); ++i) {
			auto val = begin[i];
			if (val < 256) {
				$mb[i] = static_cast<uchar>(val);
			} else {
				$mb[i] = 0;
				if (!_m) {
					_m.reset($mb.size());
					memset(_m.data(), 255, _m.size());
				}
				_m[i] = 0;
			}
		}
	}
};

template <typename Span>
inline optional<size_t> const_bytes_base<Span>::index_of(
	const bytes_pattern &pat, size_t start_pos) const {

	auto sz = $this()->size();
	auto mb_sz = pat.size();
	if (sz < mb_sz) {
		return nullopt;
	}

	auto begin = $this()->data() + start_pos;
	auto end = begin + $this()->size() - mb_sz;
	auto mb_begin = pat.masked().data();

	auto m_begin = pat.mask().data();
	if (m_begin) {
		for (auto it = begin; it != end; ++it) {
			if (bit_contains(mb_begin, it, m_begin, mb_sz)) {
				return it - begin;
			}
		}
		return nullopt;
	}

	if (sz == mb_sz) {
		if (bit_equal(begin, mb_begin, sz)) {
			return 0;
		}
		return nullopt;
	}

	size_t h = 0;
	size_t mb_h = 0;
	for (size_t i = 0; i < mb_sz; ++i) {
		mb_h += mb_begin[i];
	}

	auto mb_back_ix = mb_sz - 1;
	for (auto it = begin; it != end; ++it) {
		if (it == begin) {
			for (size_t j = 0; j < mb_sz; ++j) {
				h += it[j];
			}
		} else {
			h -= it[-1];
			h += it[mb_back_ix];
		}
		if (h != mb_h) {
			continue;
		}
		if (bit_equal(it, mb_begin, mb_sz)) {
			return it - begin;
		}
	}
	return nullopt;
}

template <typename Span>
inline optional<size_t> const_bytes_base<Span>::last_index_of(
	const bytes_pattern &pat, size_t start_pos) const {

	auto sz = $this()->size();
	auto mb_sz = pat.size();
	if (sz < mb_sz) {
		return nullopt;
	}

	if (start_pos >= sz) {
		start_pos = sz;
	}

	auto begin = $this()->data();
	auto begin_before = begin - 1;
	auto end = $this()->data() + start_pos - mb_sz;
	auto mb_begin = pat.masked().data();

	auto m_begin = pat.mask().data();
	if (m_begin) {
		for (auto it = end; it != begin_before; --it) {
			if (bit_contains(mb_begin, it, m_begin, mb_sz)) {
				return it - begin;
			}
		}
		return nullopt;
	}

	if (sz == mb_sz) {
		if (bit_equal(end, mb_begin, sz)) {
			return 0;
		}
		return nullopt;
	}

	size_t h = 0;
	size_t mb_h = 0;
	for (size_t i = 0; i < mb_sz; ++i) {
		mb_h += mb_begin[i];
	}

	for (auto it = end; it != begin_before; --it) {
		if (it == end) {
			for (size_t j = 0; j < mb_sz; ++j) {
				h += it[j];
			}
		} else {
			h -= it[mb_sz];
			h += it[0];
		}
		if (h != mb_h) {
			continue;
		}
		if (bit_equal(it, mb_begin, mb_sz)) {
			return it - begin;
		}
	}
	return nullopt;
}

template <typename Bytes>
class basic_bytes_finder : private wandering_iterator {
public:
	struct sub_area_t {
		size_t offset, size;
	};

	static basic_bytes_finder find(
		Bytes place,
		bytes_pattern pat,
		std::vector<sub_area_t> vas,
		size_t start_pos = 0) {
		auto pos_opt = place.index_of(pat, start_pos);
		if (!pos_opt) {
			return basic_bytes_finder();
		}
		return basic_bytes_finder(place, pat, std::move(vas), *pos_opt);
	}

	static basic_bytes_finder rfind(
		Bytes place,
		bytes_pattern pat,
		std::vector<sub_area_t> vas,
		size_t start_pos = nullpos) {
		auto pos_opt = place.last_index_of(pat, start_pos);
		if (!pos_opt) {
			return basic_bytes_finder();
		}
		return basic_bytes_finder(place, pat, std::move(vas), *pos_opt);
	}

	basic_bytes_finder() = default;

	operator bool() const {
		return $found.size();
	}

	Bytes &operator*() {
		return $found;
	}

	const Bytes &operator*() const {
		return $found;
	}

	Bytes *operator->() {
		return &$found;
	}

	const Bytes *operator->() const {
		return &$found;
	}

	Bytes operator[](size_t ix) {
		auto &sub = $vas[ix];
		return $found(sub.offset, sub.offset + sub.size);
	}

	bytes_view operator[](size_t ix) const {
		auto &sub = $vas[ix];
		return $found(sub.offset, sub.offset + sub.size);
	}

	basic_bytes_finder &operator++() {
		return *this = basic_bytes_finder::find(
				   $place, std::move($pat), std::move($vas), pos());
	}

	basic_bytes_finder operator++(int) {
		basic_bytes_finder old(*this);
		++*this;
		return old;
	}

	basic_bytes_finder &operator--() {
		return *this = basic_bytes_finder::rfind(
				   $place, std::move($pat), std::move($vas), pos());
	}

	basic_bytes_finder operator--(int) {
		basic_bytes_finder old(*this);
		--*this;
		return old;
	}

	size_t pos() const {
		assert($found);
		return $found.data() - $place.data();
	}

	Bytes befores() {
		return $place(0, pos());
	}

	bytes_view befores() const {
		return $place(0, pos());
	}

	Bytes afters() {
		return $place(pos() + $found.size());
	}

	bytes_view afters() const {
		return $place(pos() + $found.size());
	}

private:
	Bytes $place, $found;
	bytes_pattern $pat;
	std::vector<sub_area_t> $vas;

	basic_bytes_finder(
		Bytes place,
		bytes_pattern pat,
		std::vector<sub_area_t> vas,
		size_t found_pos) :
		$place(place),
		$found(place(found_pos, found_pos + pat.size())),
		$pat(std::move(pat)),
		$vas(std::move(vas)) {

		auto mask = $pat.mask();
		if (!mask || $vas.size()) {
			return;
		}

		auto in_va = false;
		size_t i;
		for (i = 0; i < mask.size(); ++i) {
			auto val = mask[i];
			if (val) {
				if (in_va) {
					in_va = false;
					$vas.back().size = i - $vas.back().offset;
				}
			} else if (!in_va) {
				in_va = true;
				$vas.emplace_back(sub_area_t{i, 0});
			}
		}
		if (in_va) {
			$vas.back().size = i - $vas.back().offset;
		}
	}
};

template <typename Span>
inline const_bytes_finder
const_bytes_base<Span>::find(bytes_pattern pat, size_t start_pos) const {
	return const_bytes_finder::find(*$this(), std::move(pat), {}, start_pos);
}

template <typename Span>
inline const_bytes_rfinder
const_bytes_base<Span>::rfind(bytes_pattern pat, size_t start_pos) const {
	return const_bytes_rfinder(
		const_bytes_finder::rfind(*$this(), std::move(pat), {}, start_pos));
}

template <typename Span>
inline bytes_finder
bytes_base<Span>::find(bytes_pattern pat, size_t start_pos) {
	return bytes_finder::find(*$this(), std::move(pat), {}, start_pos);
}

template <typename Span>
inline const_bytes_finder
bytes_base<Span>::find(bytes_pattern pat, size_t start_pos) const {
	return const_bytes_finder::find(*$this(), std::move(pat), {}, start_pos);
}

template <typename Span>
inline bytes_rfinder
bytes_base<Span>::rfind(bytes_pattern pat, size_t start_pos) {
	return bytes_rfinder(
		bytes_finder::rfind(*$this(), std::move(pat), {}, start_pos));
}

template <typename Span>
inline const_bytes_rfinder
bytes_base<Span>::rfind(bytes_pattern pat, size_t start_pos) const {
	return const_bytes_rfinder(
		const_bytes_finder::rfind(*$this(), std::move(pat), {}, start_pos));
}

template <typename Derived, size_t Size = size_of<Derived>::value>
class enable_bytes_accessor
	: public bytes_base<enable_bytes_accessor<Derived, Size>> {
public:
	uchar *data() {
		return reinterpret_cast<uchar *>(static_cast<Derived *>(this));
	}

	const uchar *data() const {
		return reinterpret_cast<const uchar *>(
			static_cast<const Derived *>(this));
	}

	static constexpr size_t size() {
		return Size;
	}

protected:
	constexpr enable_bytes_accessor() = default;
};

template <size_t Size, size_t Align = Size + Size % 2>
class bytes_block : public bytes_base<bytes_block<Size, Align>> {
public:
	template <typename... Bytes>
	constexpr bytes_block(Bytes... byts) : $raw{static_cast<uchar>(byts)...} {}

	uchar *data() {
		return &$raw[0];
	}

	const uchar *data() const {
		return &$raw[0];
	}

	static constexpr size_t size() {
		return Size;
	}

private:
	alignas(Align) uchar $raw[Size];
};

} // namespace rua

#endif
