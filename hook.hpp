#ifndef _RUA_HOOK_HPP
#define _RUA_HOOK_HPP

#include <MinHook.h>

#include <functional>
#include <vector>
#include <mutex>
#include <cassert>

namespace rua {
	extern size_t _hooked_count;
	extern std::mutex _hook_mtx;

	template <typename T>
	class hooked {
		public:
			using target_type = T;

			constexpr hooked() : _t(nullptr), _o(nullptr) {}

			hooked(T target, T detour) : _t(nullptr) {
				hook(target, detour);
			}

			hooked(const hooked<T> &) = delete;

			hooked<T> &operator=(const hooked<T> &) = delete;

			hooked(hooked<T> &&src) : _t(src._t), _o(src._o) {
				if (src._t) {
					src._t = nullptr;
				}
			}

			hooked<T> &operator=(hooked<T> &&src) {
				unhook();

				if (src._t) {
					_t = src._t;
					_o = src._o;

					src._t = nullptr;
				}

				return *this;
			}

			~hooked() {
				unhook();
			}

			bool hook(T target, T detour) {
				unhook();

				_hook_mtx.lock();

				if (!_hooked_count) {
					if (MH_Initialize() == MH_ERROR_MEMORY_ALLOC) {
						return false;
					}
				}

				if (MH_CreateHook(reinterpret_cast<LPVOID>(target), reinterpret_cast<LPVOID>(detour), reinterpret_cast<LPVOID *>(&_o)) != MH_OK) {
					if (!_hooked_count) {
						MH_Uninitialize();
					}
					return false;
				}

				if (MH_EnableHook(reinterpret_cast<LPVOID>(target)) != MH_OK) {
					if (!_hooked_count) {
						MH_Uninitialize();
					}
					return false;
				}

				++_hooked_count;

				_hook_mtx.unlock();

				_t = target;
				return true;
			}

			operator bool() const {
				return _t;
			}

			template <typename... A>
			auto operator()(A&&... a) const -> decltype((*reinterpret_cast<T>(0))(std::forward<A>(a)...)) {
				return _o(std::forward<A>(a)...);
			}

			void unhook() {
				if (_t) {
					MH_DisableHook(reinterpret_cast<LPVOID>(_t));
					MH_RemoveHook(reinterpret_cast<LPVOID>(_t));
					_t = nullptr;

					_hook_mtx.lock();
					if (!--_hooked_count) {
						MH_Uninitialize();
					}
					_hook_mtx.unlock();
				}
			}

		private:
			T _t, _o;
	};
}

#endif
