#ifndef _RUA_BYTES_HPP
#define _RUA_BYTES_HPP

#include "macros.hpp"
#include "any_ptr.hpp"
#include "nullpos.hpp"

#include <cstdint>
#include <vector>
#include <cassert>

namespace rua {
	namespace bytes {
		template <typename Data>
		class operation {
			public:
				template <typename RelPtr>
				any_ptr derel(ptrdiff_t pos = 0) const {
					return _this()->base() + pos + _this()->template get<RelPtr>(pos) + sizeof(RelPtr);
				}

				size_t match(const std::vector<uint16_t> &pattern) const {
					if (!pattern.size()) {
						return nullpos;
					}
					size_t end = _this()->size() ? _this()->size() + 1 - pattern.size() : 0;
					for (size_t i = 0; i < end || !end; ++i) {
						size_t j;
						for (j = 0; j < pattern.size(); ++j) {
							if (pattern[j] > 255) {
								continue;
							}
							if (pattern[j] != _this()->template get<uint8_t>(i + j)) {
								break;
							}
						}
						if (j == pattern.size()) {
							return i;
						}
					}
					return nullpos;
				}

				std::vector<size_t> match_sub(const std::vector<uint16_t> &pattern) const {
					if (!pattern.size()) {
						return std::vector<size_t>();
					}
					std::vector<size_t> subs;
					bool in_sub = false;
					size_t end = _this()->size() ? _this()->size() + 1 - pattern.size() : 0;
					for (size_t i = 0; i < end || !end; ++i) {
						size_t j;
						for (j = 0; j < pattern.size(); ++j) {
							if (pattern[j] > 255) {
								if (!in_sub) {
									subs.emplace_back(i + j);
									in_sub = true;
								}
								continue;
							}
							if (pattern[j] != _this()->template get<uint8_t>(i + j)) {
								break;
							}
							if (in_sub) {
								in_sub = false;
							}
						}
						if (j == pattern.size()) {
							return subs;
						}
						subs.clear();
						in_sub = false;
					}
					return subs;
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
