#ifndef _RUA_BYTES_HPP
#define _RUA_BYTES_HPP

#include "bit.hpp"
#include "macros.hpp"
#include "optional.hpp"
#include "span.hpp"
#include "string/len.hpp"
#include "string/view.hpp"
#include "types/traits.hpp"
#include "types/util.hpp"

#include <cassert>
#include <cstring>
#include <string>
#include <vector>

namespace rua {

class bytes_view;
class bytes_ref;
class bytes;
class masked_bytes;

template <typename Span>
class const_bytes_base {
public:
	generic_ptr data_generic() const {
		return _this()->data();
	}

	template <typename T>
	T get() const {
		return bit_get<T>(_this()->data());
	}

	template <typename T>
	T get(ptrdiff_t offset) const {
		return bit_get<T>(_this()->data(), offset);
	}

	template <typename T>
	T aligned_get(ptrdiff_t ix) const {
		return bit_aligned_get<T>(_this()->data(), ix);
	}

	template <typename T>
	const T &as() const {
		return bit_as<const T>(_this()->data());
	}

	template <typename T>
	const T &as(ptrdiff_t offset) const {
		return bit_as<const T>(_this()->data(), offset);
	}

	template <typename T>
	const T &aligned_as(ptrdiff_t ix) const {
		return bit_aligned_as<const T>(_this()->data(), ix);
	}

	const byte &operator[](ptrdiff_t offset) const {
		return as<const byte>(offset);
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
	operator basic_string_view<CharT, Traits>() const {
		return basic_string_view<CharT, Traits>(
			data_generic().template as<const CharT *>(),
			_this()->size() / sizeof(CharT));
	}

	template <typename CharT, typename Traits, typename Allocator>
	operator std::basic_string<CharT, Traits, Allocator>() const {
		return std::basic_string<CharT, Traits, Allocator>(
			data_generic().template as<const CharT *>(),
			_this()->size() / sizeof(CharT));
	}

	template <typename RelPtr, size_t SlotSize = sizeof(RelPtr)>
	generic_ptr derel(ptrdiff_t pos = 0) const {
		return _this()->data() + pos + SlotSize + get<RelPtr>(pos);
	}

	inline bool eq(bytes_view) const;

	inline bool operator==(bytes_view) const;

	inline bool has(const masked_bytes &) const;

	inline optional<size_t> find_pos(bytes_view) const;

	inline bytes_view find(bytes_view) const;

	inline optional<size_t> match_pos(const masked_bytes &) const;

	inline std::vector<bytes_view> match(const masked_bytes &) const;

protected:
	const_bytes_base() = default;

private:
	const Span *_this() const {
		return static_cast<const Span *>(this);
	}
};

template <typename Span>
class bytes_base : public const_bytes_base<Span> {
public:
	template <typename T>
	void set(const T &val) {
		return bit_set<T>(_this()->data(), val);
	}

	template <typename T>
	void set(const T &val, ptrdiff_t offset) {
		return bit_set<T>(_this()->data(), offset, val);
	}

	template <typename T>
	void aligned_set(const T &val, ptrdiff_t ix) {
		return bit_aligned_set<T>(_this()->data(), val, ix);
	}

	template <typename T>
	const T &as() const {
		return bit_as<const T>(_this()->data());
	}

	template <typename T>
	T &as() {
		return bit_as<T>(_this()->data());
	}

	template <typename T>
	const T &as(ptrdiff_t offset) const {
		return bit_as<const T>(_this()->data(), offset);
	}

	template <typename T>
	T &as(ptrdiff_t offset) {
		return bit_as<T>(_this()->data(), offset);
	}

	template <typename T>
	const T &aligned_as(ptrdiff_t ix) const {
		return bit_aligned_as<const T>(_this()->data(), ix);
	}

	template <typename T>
	T &aligned_as(ptrdiff_t ix) {
		return bit_aligned_as<T>(_this()->data(), ix);
	}

	const byte &operator[](ptrdiff_t offset) const {
		return as<const byte>(offset);
		;
	}

	byte &operator[](ptrdiff_t offset) {
		return as<byte>(offset);
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
	Span *_this() {
		return static_cast<Span *>(this);
	}

	const Span *_this() const {
		return static_cast<const Span *>(this);
	}
};

class bytes_view : public const_bytes_base<bytes_view> {
public:
	constexpr bytes_view() : _p(nullptr), _n(0) {}

	constexpr bytes_view(std::nullptr_t) : bytes_view() {}

	template <
		typename T,
		typename DecayT = decay_t<T>,
		typename = enable_if_t<std::is_base_of<generic_ptr, DecayT>::value>>
	constexpr bytes_view(T &&ptr, size_t size) : _p(ptr), _n(ptr ? size : 0) {}

	constexpr bytes_view(const byte *ptr, size_t size) :
		_p(ptr), _n(ptr ? size : 0) {}

	bytes_view(const void *ptr, size_t size) :
		_p(reinterpret_cast<const byte *>(ptr)), _n(ptr ? size : 0) {}

	template <
		typename T,
		typename = enable_if_t<
			size_of<remove_cv_t<T>>::value &&
			!std::is_convertible<T *, string_view>::value &&
			!std::is_convertible<T *, wstring_view>::value>>
	bytes_view(T *ptr, size_t size = sizeof(T)) :
		_p(reinterpret_cast<const byte *>(ptr)), _n(ptr ? size : 0) {}

	template <typename R, typename... Args>
	bytes_view(R (*fn_ptr)(Args...), size_t size) :
		_p(reinterpret_cast<const byte *>(fn_ptr)), _n(fn_ptr ? size : 0) {}

	template <typename T, typename R, typename... Args>
	bytes_view(R (T::*mem_fn_ptr)(Args...), size_t size) :
		_p(bit_cast<const byte *>(mem_fn_ptr)), _n(mem_fn_ptr ? size : 0) {}

	template <
		typename T,
		typename = enable_if_t<
			!std::is_convertible<T &&, generic_ptr>::value &&
			!is_span<T &&>::value>>
	bytes_view(T &&ref, size_t size = sizeof(T)) :
		_p(reinterpret_cast<const byte *>(&ref)), _n(size) {}

	bytes_view(std::initializer_list<byte> il) :
		bytes_view(reinterpret_cast<const void *>(il.begin()), il.size()) {}

	bytes_view(const char *c_str, size_t size) :
		_p(reinterpret_cast<const byte *>(c_str)), _n(size) {}

	bytes_view(const char *c_str) :
		_p(reinterpret_cast<const byte *>(c_str)), _n(c_str_len(c_str)) {}

	bytes_view(const wchar_t *c_wstr, size_t size) :
		_p(reinterpret_cast<const byte *>(c_wstr)), _n(size) {}

	bytes_view(const wchar_t *c_wstr) :
		_p(reinterpret_cast<const byte *>(c_wstr)),
		_n(c_str_len(c_wstr) * sizeof(wchar_t)) {}

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
			reinterpret_cast<const byte *>(rua::data(std::forward<T>(span))),
			rua::size(std::forward<T>(span)) *
				sizeof(typename SpanTraits::element_type)) {}

	constexpr operator bool() const {
		return _p;
	}

	const byte *data() const {
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
	const byte *_p;
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
inline bytes_view const_bytes_base<Span>::operator()(
	ptrdiff_t begin_offset, ptrdiff_t end_offset_from_begin) const {
	return slice(begin_offset, end_offset_from_begin);
}

template <typename Span>
inline bytes_view
const_bytes_base<Span>::operator()(ptrdiff_t begin_offset) const {
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
inline optional<size_t>
const_bytes_base<Span>::find_pos(bytes_view target) const {
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

	template <
		typename T,
		typename DecayT = decay_t<T>,
		typename = enable_if_t<std::is_base_of<generic_ptr, DecayT>::value>>
	constexpr bytes_ref(T &&ptr, size_t size) : _p(ptr), _n(ptr ? size : 0) {}

	constexpr bytes_ref(byte *ptr, size_t size) : _p(ptr), _n(ptr ? size : 0) {}

	bytes_ref(void *ptr, size_t size) :
		_p(reinterpret_cast<byte *>(ptr)), _n(ptr ? size : 0) {}

	template <
		typename T,
		typename = enable_if_t<
			size_of<remove_cv_t<T>>::value && !std::is_const<T>::value &&
			!std::is_convertible<T *, string_view>::value &&
			!std::is_convertible<T *, wstring_view>::value>>
	bytes_ref(T *ptr, size_t size = sizeof(T)) :
		_p(reinterpret_cast<byte *>(ptr)), _n(ptr ? size : 0) {}

	template <
		typename T,
		typename = enable_if_t<
			!std::is_const<remove_reference_t<T>>::value &&
			!std::is_convertible<T &&, generic_ptr>::value &&
			!is_span<T &&>::value>>
	bytes_ref(T &&ref, size_t size = sizeof(T)) :
		_p(reinterpret_cast<byte *>(&ref)), _n(size) {}

	bytes_ref(char *c_str, size_t size) :
		_p(reinterpret_cast<byte *>(c_str)), _n(size) {}

	bytes_ref(char *c_str) :
		_p(reinterpret_cast<byte *>(c_str)), _n(c_str_len(c_str)) {}

	bytes_ref(wchar_t *c_wstr, size_t size) :
		_p(reinterpret_cast<byte *>(c_wstr)), _n(size) {}

	bytes_ref(wchar_t *c_wstr) :
		_p(reinterpret_cast<byte *>(c_wstr)),
		_n(c_str_len(c_wstr) * sizeof(wchar_t)) {}

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
			reinterpret_cast<byte *>(rua::data(std::forward<T>(span))),
			rua::size(std::forward<T>(span)) *
				sizeof(typename SpanTraits::element_type)) {}

	constexpr operator bool() const {
		return _p;
	}

	byte *data() {
		return _p;
	}

	const byte *data() const {
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
	byte *_p;
	size_t _n;
};

inline bytes_ref as_bytes_ref(bytes_view src) {
	return bytes_ref(const_cast<byte *>(src.data()), src.size());
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
inline bytes_view bytes_base<Span>::operator()(
	ptrdiff_t begin_offset, ptrdiff_t end_offset_from_begin) const {
	return slice(begin_offset, end_offset_from_begin);
}

template <typename Span>
inline bytes_view bytes_base<Span>::operator()(ptrdiff_t begin_offset) const {
	return slice(begin_offset);
}

template <typename Span>
inline bytes_ref bytes_base<Span>::operator()(
	ptrdiff_t begin_offset, ptrdiff_t end_offset_from_begin) {
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

	bytes &operator=(const bytes &val) {
		reset(val);
		return *this;
	}

	bytes &operator=(bytes &&val) {
		reset(std::move(val));
		return *this;
	}

	template <typename T>
	enable_if_t<
		!std::is_base_of<bytes, decay_t<T>>::value &&
			std::is_convertible<T &&, bytes_view>::value,
		bytes &>
	operator=(T &&val) {
		reset(bytes_view(std::forward<T>(val)));
		return *this;
	}

	bytes &operator+=(bytes_view tail) {
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
	void _alloc(size_t size) {
		bytes_ref::reset(
			new char[sizeof(size_t) + size] + sizeof(size_t), size);
		bit_set<size_t>(data() - sizeof(size_t), size);
	}

	size_t _capacity() const {
		return bit_get<size_t>(data() - sizeof(size_t));
	}
};

template <
	typename A,
	typename B,
	typename DecayA = decay_t<A>,
	typename DecayB = decay_t<B>>
inline enable_if_t<
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
inline enable_if_t<
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
inline enable_if_t<
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
	masked_bytes() : _n(0) {}

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
	return bit_has(
		_this()->data(), target.masked().data(), target.mask().data(), sz);
}

template <typename Span>
inline optional<size_t>
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

template <
	typename Derived,
	size_t Size = !std::is_same<Derived, void>::value ? size_of<Derived>::value
													  : nmax<size_t>()>
class enable_bytes_accessor
	: public bytes_base<enable_bytes_accessor<Derived, Size>> {
public:
	byte *data() {
		return reinterpret_cast<const byte *>(
			static_cast<const Derived *>(this));
	}

	const byte *data() const {
		return reinterpret_cast<const byte *>(
			static_cast<const Derived *>(this));
	}

	static constexpr size_t size() {
		return Size;
	}

protected:
	constexpr enable_bytes_accessor() = default;
};

template <>
inline byte *enable_bytes_accessor<void>::data() {
	return reinterpret_cast<byte *>(this);
}

template <>
inline const byte *enable_bytes_accessor<void>::data() const {
	return reinterpret_cast<const byte *>(this);
}

template <size_t Size, size_t Align = Size + Size % 2>
class bytes_block : public bytes_base<bytes_block<Size, Align>> {
public:
	byte *data() {
		return &_raw[0];
	}

	const byte *data() const {
		return &_raw[0];
	}

	static constexpr size_t size() {
		return Size;
	}

private:
	alignas(Align) byte _raw[Size];
};

} // namespace rua

#endif
