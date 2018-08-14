#ifndef _RUA_BIN_HPP
#define _RUA_BIN_HPP

#include "mem/get.hpp"

#include "macros.hpp"
#include "bytes.hpp"
#include "limits.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <atomic>
#include <cstring>
#include <utility>
#include <cassert>

namespace rua {
	template <typename Getter>
	class bin_base : public bytes::base<Getter> {
		public:
			template <typename RelPtr, size_t SlotSize = sizeof(RelPtr)>
			any_ptr derel(ptrdiff_t pos = 0) const {
				return _this()->ptr() + pos + SlotSize + _this()->template get<RelPtr>(pos);
			}

			template <typename RelPtr, size_t SlotSize = sizeof(RelPtr)>
			RelPtr enrel(ptrdiff_t pos, any_ptr abs_ptr) {
				auto rel_ptr = static_cast<RelPtr>(abs_ptr - (_this()->ptr() + pos + SlotSize));
				_this()->template set<RelPtr>(pos, rel_ptr);
				return rel_ptr;
			}

			template <typename RelPtr, size_t SlotSize = sizeof(RelPtr)>
			RelPtr enrel(any_ptr abs_ptr) {
				return enrel<RelPtr, SlotSize>(0, abs_ptr);
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

	class bin : public bin_base<bin> {
		public:
			constexpr bin() : bin(nullptr, 0, nullptr) {}

			constexpr bin(std::nullptr_t) : bin() {}

			explicit bin(size_t byte_count) {
				if (byte_count) {
					_alloc(byte_count);
				} else {
					_clear();
				}
			}

			template <typename T, typename = typename std::enable_if<!std::is_same<T, void>::value>::type>
			constexpr bin(T *ptr, size_t element_count = 1) : bin(ptr, element_count * sizeof(T), nullptr) {}

			constexpr bin(any_ptr ptr, size_t byte_count = nmax<size_t>()) : bin(ptr, byte_count, nullptr) {}

			~bin() {
				free();
			}

			void free() {
				if (_alloced) {
					if (!--_alloced_use_count()) {
						std::free(_alloced);
					}
				}
				_clear(0);
			}

			bin(const bin &src) : bin(src._sz) {
				copy(src);
			}

			bin &operator=(const bin &src) {
				free();
				if (src) {
					_alloc(src._sz);
					copy(src);
				}
				return *this;
			}

			bin(bin &&src) {
				if (src) {
					_ptr = src._ptr;
					_sz = src._sz;
					_alloced = src._alloced;
					src._clear(0);
				}
			}

			bin &operator=(bin &&src) {
				free();
				if (src) {
					_ptr = src._ptr;
					_sz = src._sz;
					_alloced = src._alloced;
					src._clear(0);
				}
				return *this;
			}

			operator bool() const {
				return _ptr;
			}

			any_ptr ptr() const {
				return _ptr;
			}

			size_t size() const {
				assert(_sz ? _ptr : true);

				return _sz;
			}

			size_t copy(const bin &src) {
				auto cp_sz = _sz < src._sz ? _sz : src._sz;
				if (!cp_sz) {
					return 0;
				}
				memcpy(_ptr, src._ptr, cp_sz);
				return cp_sz;
			}

			template <typename D>
			D &get(ptrdiff_t offset = 0) {
				return mem::get<D>(_ptr, offset);
			}

			template <typename D>
			const D &get(ptrdiff_t offset = 0) const {
				return mem::get<D>(_ptr, offset);
			}

			template <typename D>
			D &aligned_get(ptrdiff_t ix = 0) {
				return get<D>(ix * sizeof(D));
			}

			template <typename D>
			const D &aligned_get(ptrdiff_t ix = 0) const {
				return get<D>(ix * sizeof(D));
			}

			uint8_t &operator[](ptrdiff_t offset) {
				return get<uint8_t>(offset);
			}

			const uint8_t &operator[](ptrdiff_t offset) const {
				return get<uint8_t>(offset);
			}

			bin slice(ptrdiff_t begin, ptrdiff_t end) const {
				assert(end > begin);
				assert(_sz <= static_cast<size_t>(nmax<ptrdiff_t>()));

				auto _szi = static_cast<ptrdiff_t>(_sz);
				if (end > _szi) {
					end = _szi;
				}

				auto new_ptr = _ptr + begin;
				auto new_sz = static_cast<size_t>(end - begin);

				if (_alloced) {
					#ifndef NDEBUG
						assert(new_ptr >= _alloced_base());
						assert(new_sz <= _alloced_size());
					#endif

					++_alloced_use_count();
				}

				return bin(new_ptr, new_sz, _alloced);
			}

			bin slice(ptrdiff_t begin) const {
				auto new_ptr = _ptr + begin;
				auto new_sz = static_cast<size_t>(static_cast<ptrdiff_t>(_sz) - begin);

				if (_alloced) {
					#ifndef NDEBUG
						assert(new_ptr >= _alloced_base());
						assert(new_sz <= _alloced_size());
					#endif

					++_alloced_use_count();
				}

				return bin(new_ptr, new_sz, _alloced);
			}

			bin operator()(ptrdiff_t begin, ptrdiff_t end) const {
				return slice(begin, end);
			}

			bin operator()(ptrdiff_t begin) const {
				return slice(begin);
			}

			bin operator()() const {
				return *this;
			}

			bool is_unique() const {
				return _alloced ? _alloced_use_count().load() == 1 : false;
			}

			bin &to_unique() {
				if (is_unique()) {
					return *this;
				}
				*this = bin(*this);
				return *this;
			}

			bin original_alloced() const {
				return _alloced ?
					bin(_alloced_base(), _alloced_size(), _alloced) :
					bin(_ptr, _sz, nullptr)
				;
			}

		private:
			any_ptr _ptr;
			size_t _sz;
			any_ptr _alloced;

			constexpr bin(any_ptr base, size_t size, any_ptr alloced) : _ptr(base), _sz(size), _alloced(alloced) {}

			std::atomic<size_t> &_alloced_use_count() const {
				return *_alloced.to<std::atomic<size_t> *>();
			}

			size_t &_alloced_size() const {
				return *(_alloced + sizeof(std::atomic<size_t>)).to<size_t *>();
			}

			any_ptr _alloced_base() const {
				return _alloced + sizeof(std::atomic<size_t>) + sizeof(size_t);
			}

			void _alloc(size_t size) {
				_alloced = malloc(sizeof(std::atomic<size_t>) + sizeof(size_t) + size);
				new (&_alloced_use_count()) std::atomic<size_t>(1);
				_alloced_size() = size;

				_ptr = _alloced_base();
				_sz = size;
			}

			void _clear(size_t sz = nmax<size_t>()) {
				_ptr = nullptr;
				_sz = sz;
				_alloced = nullptr;
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
