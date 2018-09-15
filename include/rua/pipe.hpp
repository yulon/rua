#ifndef _RUA_PIPE_HPP
#define _RUA_PIPE_HPP

#include "io/sys.hpp"
#include "strenc.hpp"
#include "limits.hpp"

#ifdef _WIN32
	#include <windows.h>
#endif

#include <string>
#include <cstring>
#include <cassert>

namespace rua {
	struct pipe_accessors {
		io::sys::read_closer r;
		io::sys::write_closer w;

		operator bool() const {
			return r || w;
		}
	};

	#if defined(_WIN32)
		inline bool make_pipe(io::sys::read_closer &r, io::sys::write_closer &w) {
			SECURITY_ATTRIBUTES sa;
			memset(&sa, 0, sizeof(SECURITY_ATTRIBUTES));
			sa.nLength = sizeof(SECURITY_ATTRIBUTES);

			if (!CreatePipe(&r.native_handle(), &w.native_handle(), &sa, 0)) {
				r.detach();
				w.detach();
				return false;
			}
			return true;
		}

		inline pipe_accessors make_pipe() {
			pipe_accessors pa;
			make_pipe(pa.r, pa.w);
			return pa;
		}

		inline io::sys::read_write_closer make_pipe(const std::string &name, size_t timeout = nmax<size_t>()) {
			auto h = CreateNamedPipeW(
				rua::u8_to_w("\\\\.\\pipe\\" + name).c_str(),
				PIPE_ACCESS_INBOUND | PIPE_ACCESS_OUTBOUND,
				PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
				PIPE_UNLIMITED_INSTANCES,
				0,
				0,
				static_cast<DWORD>(timeout),
				0
			);
			if (!h) {
				return nullptr;
			}
			if (!ConnectNamedPipe(h, nullptr)) {
				return nullptr;
			}
			return h;
		}

		inline io::sys::read_write_closer open_pipe(const std::string &name, size_t timeout = nmax<size_t>()) {
			auto fmt_name = rua::u8_to_w("\\\\.\\pipe\\" + name);

			if (!WaitNamedPipeW(fmt_name.c_str(), static_cast<DWORD>(timeout))) {
				return nullptr;
			}
			return CreateFileW(
				fmt_name.c_str(),
				GENERIC_READ | GENERIC_WRITE,
				0,
				nullptr,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				nullptr
			);
		}
	#else
		#error rua::pipe: not supported this platform!
	#endif
}

#endif
