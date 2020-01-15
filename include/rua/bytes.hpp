#ifndef _RUA_BYTES_HPP
#define _RUA_BYTES_HPP

#include "bit.hpp"
#include "byte.hpp"
#include "limits.hpp"
#include "macros.hpp"
#include "opt.hpp"
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
class bytes;
class masked_bytes;

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
		return at<const byte>(offset);
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

	inline bool eq(bytes_view) const;

	inline bool operator==(bytes_view) const;

	inline bool has(const masked_bytes &) const;

	inline opt<size_t> find_pos(bytes_view) const;

	inline bytes_view find(bytes_view) const;

	inline opt<size_t> match_pos(const masked_bytes &) const;

	inline std::vector<bytes_view> match(const masked_bytes &) const;

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
		return at<const byte>(offset);
		;
	}

	RUA_FORCE_INLINE byte &operator[](ptrdiff_t offset) {
		return at<byte>(offset);
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

	inline bytes_view find(bytes_view target) const;

	inline bytes_ref find(bytes_view target);

	inline std::vector<bytes_view> match(const masked_bytes &) const;

	inline std::vector<bytes_ref> match(const masked_bytes &);

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

	bytes_view(std::initializer_list<const byte> il) :
		_p(il.begin()),
		_n(il.size()) {}

	bytes_view(const char *c_str, size_t size) : _p(c_str), _n(size) {}

	bytes_view(const char *c_str) : _p(c_str), _n(c_str ? str_len(c_str) : 0) {}

	bytes_view(const wchar_t *c_wstr, size_t size) : _p(c_wstr), _n(size) {}

	bytes_view(const wchar_t *c_wstr) :
		_p(c_wstr),
		_n(c_wstr ? str_len(c_wstr) * sizeof(wchar_t) : 0) {}

	template <
		typename T,
		typename = enable_if_t<
			!std::is_same<T, const byte>::value &&
			!std::is_convertible<T *, string_view>::value &&
			!std::is_convertible<T *, wstring_view>::value &&
			!std::is_function<T>::value>>
	bytes_view(T *ptr, size_t size = sizeof(T)) : _p(ptr), _n(ptr ? size : 0) {}

	template <typename R, typename... A>
	bytes_view(R (*ptr)(A...), size_t size = nmax<size_t>()) :
		_p(ptr),
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
inline bool const_bytes_base<Span>::eq(bytes_view target) const {
	auto sz = _this()->size();
	if (sz != target.size()) {
		return false;
	}
	const byte *a = _this()->data();
	const byte *b = target.data();
	if (a == b) {
		return true;
	}
	if (!a || !b) {
		return false;
	}
	return bit_eq(a, b, sz);
}

template <typename Span>
inline bool const_bytes_base<Span>::operator==(bytes_view target) const {
	return eq(target);
}

template <typename Span>
inline opt<size_t> const_bytes_base<Span>::find_pos(bytes_view target) const {
	auto sz = _this()->size();
	auto tg_sz = target.size();
	if (sz < tg_sz) {
		return nullopt;
	}

	const byte *begin = _this()->data();
	const byte *tg_begin = target.data();
	if (sz == tg_sz) {
		if (bit_eq(begin, tg_begin, sz)) {
			return 0;
		}
		return nullopt;
	}
	auto end = begin + _this()->size() - tg_sz;
	auto tg_back_ix = tg_sz - 1;

	size_t tg_h = 0;
	for (size_t i = 0; i < tg_sz; ++i) {
		tg_h += tg_begin[i];
	}
	size_t h = 0;
	for (auto it = begin; it != end; ++it) {
		if (it == begin) {
			for (size_t j = 0; j < tg_sz; ++j) {
				h += it[j];
			}
		} else {
			h -= it[-1];
			h += it[tg_back_ix];
		}
		if (h != tg_h) {
			continue;
		}
		if (bit_eq(it, tg_begin, tg_sz)) {
			return it - begin;
		}
	}
	return nullopt;
}

template <typename Span>
inline bytes_view const_bytes_base<Span>::find(bytes_view target) const {
	auto pos_opt = find_pos(target);
	return pos_opt
			   ? bytes_view(_this()->data() + pos_opt.value(), target.size())
			   : nullptr;
}

class bytes_ref : public bytes_base<bytes_ref> {
public:
	constexpr bytes_ref() : _p(nullptr), _n(0) {}

	constexpr bytes_ref(std::nullptr_t) : bytes_ref() {}

	constexpr bytes_ref(generic_ptr ptr, size_t size) :
		_p(ptr),
		_n(ptr ? size : 0) {}

	bytes_ref(std::initializer_list<byte> il) : _p(il.begin()), _n(il.size()) {}

	bytes_ref(char *c_str, size_t size) : _p(c_str), _n(size) {}

	bytes_ref(char *c_str) : _p(c_str), _n(c_str ? str_len(c_str) : 0) {}

	bytes_ref(wchar_t *c_wstr, size_t size) : _p(c_wstr), _n(size) {}

	bytes_ref(wchar_t *c_wstr) :
		_p(c_wstr),
		_n(str_len(c_wstr) * sizeof(wchar_t)) {}

	template <
		typename T,
		typename = enable_if_t<
			!std::is_const<T>::value && !std::is_same<T, byte>::value &&
			!std::is_convertible<T *, string_view>::value &&
			!std::is_convertible<T *, wstring_view>::value &&
			!std::is_function<T>::value>>
	bytes_ref(T *ptr, size_t size = sizeof(T)) : _p(ptr), _n(ptr ? size : 0) {}

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

inline bytes_ref as_bytes_ref(bytes_view src) {
	return bytes_ref(src.data(), src.size());
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

template <typename Span>
inline bytes_view bytes_base<Span>::find(bytes_view target) const {
	auto pos_opt = const_bytes_base<Span>::find_pos(target);
	return pos_opt
			   ? bytes_view(_this()->data() + pos_opt.value(), target.size())
			   : nullptr;
}

template <typename Span>
inline bytes_ref bytes_base<Span>::find(bytes_view target) {
	auto pos_opt = const_bytes_base<Span>::find_pos(target);
	return pos_opt ? bytes_ref(_this()->data() + pos_opt.value(), target.size())
				   : nullptr;
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

	bytes(std::initializer_list<const byte> il) :
		bytes(il.begin(), il.size()) {}

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

class masked_bytes : public bytes {
public:
	masked_bytes(std::initializer_list<uint16_t> decl_ary) {
		_input(decl_ary);
	}

	size_t size() const {
		return _n;
	}

	bytes_view masked() const {
		return _masked;
	}

	bytes_view mask() const {
		return _mask;
	}

	struct sub_t {
		size_t offset, size;
	};

	const std::vector<sub_t> &subs() const {
		return _subs;
	}

private:
	size_t _n;
	bytes _masked, _mask;
	std::vector<sub_t> _subs;

	template <typename Container>
	void _input(const Container &decl_ary) {
		_n = decl_ary.size();
		_masked.reset(_n);
		_mask.reset(_n);
		auto begin = decl_ary.begin();
		auto in_vb = false;
		size_t i;
		for (i = 0; i < _n; ++i) {
			auto val = begin[i];
			if (val < 256) {
				if (in_vb) {
					in_vb = false;
					_subs.back().size = i - _subs.back().offset;
				}
				assert(i < _n);
				_masked[i] = static_cast<byte>(val);
				_mask[i] = 255;
			} else {
				if (!in_vb) {
					in_vb = true;
					_subs.emplace_back(sub_t{i, 0});
				}
				assert(i < _n);
				_masked[i] = 0;
				_mask[i] = 0;
			}
		}
		if (in_vb) {
			_subs.back().size = i - _subs.back().offset;
		}
	}
};

template <typename Span>
inline bool const_bytes_base<Span>::has(const masked_bytes &target) const {
	auto sz = _this()->size();
	if (sz != target.size()) {
		return false;
	}
	const byte *a = _this()->data();
	const byte *b = target.data();
	if (a == b) {
		return true;
	}
	if (!a || !b) {
		return false;
	}
	return bit_has(a, b, sz);
}

template <typename Span>
inline opt<size_t>
const_bytes_base<Span>::match_pos(const masked_bytes &target) const {
	auto sz = _this()->size();
	auto tg_sz = target.size();
	if (sz < tg_sz) {
		return nullopt;
	}

	const byte *begin = _this()->data();
	const byte *tg_begin = target.masked().data();
	const byte *m_begin = target.mask().data();
	if (sz == tg_sz) {
		if (bit_has(begin, tg_begin, m_begin, sz)) {
			return 0;
		}
		return nullopt;
	}
	auto end = begin + _this()->size() - tg_sz;

	for (auto it = begin; it != end; ++it) {
		if (bit_has(it, tg_begin, m_begin, tg_sz)) {
			return it - begin;
		}
	}
	return nullopt;
}

template <typename Span>
inline std::vector<bytes_view>
const_bytes_base<Span>::match(const masked_bytes &target) const {
	std::vector<bytes_view> mr;
	auto pos_opt = match_pos(target);
	if (!pos_opt) {
		return mr;
	}
	const auto &subs = target.subs();
	mr.reserve(1 + subs.size());
	auto whole = slice(pos_opt.value(), pos_opt.value() + target.size());
	mr.emplace_back(whole);
	for (auto &sub : subs) {
		mr.emplace_back(whole(sub.offset, sub.offset + sub.size));
	}
	return mr;
}

template <typename Span>
inline std::vector<bytes_view>
bytes_base<Span>::match(const masked_bytes &target) const {
	return const_bytes_base<Span>::match(target);
}

template <typename Span>
inline std::vector<bytes_ref>
bytes_base<Span>::match(const masked_bytes &target) {
	std::vector<bytes_ref> mr;
	auto pos_opt = const_bytes_base<Span>::match_pos(target);
	if (!pos_opt) {
		return mr;
	}
	const auto &subs = target.subs();
	mr.reserve(1 + subs.size());
	auto whole = slice(pos_opt.value(), pos_opt.value() + target.size());
	mr.emplace_back(whole);
	for (auto &sub : subs) {
		mr.emplace_back(whole(sub.offset, sub.offset + sub.size));
	}
	return mr;
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
