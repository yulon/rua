#ifndef _RUA_FILE_WIN32_HPP
#define _RUA_FILE_WIN32_HPP

#include "base.hpp"

#include "../path.hpp"
#include "../range.hpp"
#include "../string/char_enc/base/win32.hpp"
#include "../string/join.hpp"
#include "../sys/stream/win32.hpp"
#include "../time/now/win32.hpp"
#include "../types/traits.hpp"
#include "../types/util.hpp"

#include <windows.h>

#include <cassert>
#include <cstring>
#include <string>

namespace rua { namespace win32 {

class file_path : public path_base<file_path, '\\'> {
public:
	RUA_PATH_CTORS(file_path)

	bool is_exist() const {
		auto path_w = u8_to_w("\\\\?\\" + abs().str());

		return GetFileAttributesW(path_w.c_str()) != INVALID_FILE_ATTRIBUTES;
	}

	bool is_dir() const {
		auto path_w = u8_to_w("\\\\?\\" + abs().str());

		auto fa = GetFileAttributesW(path_w.c_str());
		return fa != INVALID_FILE_ATTRIBUTES ? fa & FILE_ATTRIBUTE_DIRECTORY
											 : false;
	}

	file_path abs() const & {
		const auto &s = str();
		if (s.size() > 1) {
			switch (s[1]) {
			case ':':
				return *this;
			case '\\':
				if (s[0] == '\\') {
					return *this;
				}
			}
		}
		auto rel_w = u8_to_w(s);
		auto abs_w_len = GetFullPathNameW(rel_w.c_str(), 0, nullptr, nullptr);
		if (!abs_w_len) {
			return *this;
		}
		return _abs(rel_w.c_str(), abs_w_len);
	}

	file_path abs() && {
		const auto &s = str();
		if (s.size() > 1) {
			switch (s[1]) {
			case ':':
				return std::move(*this);
			case '\\':
				if (s[0] == '\\') {
					return std::move(*this);
				}
			}
		}
		auto rel_w = u8_to_w(s);
		auto abs_w_len = GetFullPathNameW(rel_w.c_str(), 0, nullptr, nullptr);
		if (!abs_w_len) {
			return std::move(*this);
		}
		return _abs(rel_w.c_str(), abs_w_len);
	}

private:
	static std::string _abs(const WCHAR *rel_c_wstr, size_t abs_w_len) {
		auto buf_sz = abs_w_len + 1;
		auto abs_c_wstr = new WCHAR[buf_sz];

		abs_w_len = GetFullPathNameW(
			rel_c_wstr, static_cast<DWORD>(abs_w_len), abs_c_wstr, nullptr);

		auto r = w_to_u8(wstring_view(abs_c_wstr, abs_w_len));
		delete[] abs_c_wstr;
		return r;
	}
};

namespace _wkdir {

inline file_path working_dir() {
	auto w_len = GetCurrentDirectoryW(0, nullptr);
	if (!w_len) {
		return "";
	}

	auto buf_sz = w_len + 1;
	auto c_wstr = new WCHAR[buf_sz];
	w_len = GetCurrentDirectoryW(buf_sz, c_wstr);

	auto r = w_to_u8(wstring_view(c_wstr, w_len));
	delete[] c_wstr;
	return r;
}

inline bool work_at(const file_path &path) {
#ifndef NDEBUG
	auto r =
#else
	return
#endif
		SetCurrentDirectoryW(u8_to_w(path.abs().str()).c_str());
#ifndef NDEBUG
	assert(r);
	return r;
#endif
}

} // namespace _wkdir

using namespace _wkdir;

template <typename T>
class basic_file_info {
public:
	using native_data_t = T;

	////////////////////////////////////////////////////////////////////////

	basic_file_info() = default;

	native_data_t &native_data() {
		return _data;
	}

	const native_data_t &native_data() const {
		return _data;
	}

	bool is_dir() const {
		return _data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
	}

	uint64_t size() const {
		return static_cast<uint64_t>(_data.nFileSizeHigh) << 32 |
			   static_cast<uint64_t>(_data.nFileSizeLow);
	}

	file_times times(int8_t zone = local_time_zone()) const {
		return {modified_time(zone), creation_time(zone), access_time(zone)};
	}

	time modified_time(int8_t zone = local_time_zone()) const {
		return from_sys_time(_data.ftLastWriteTime, zone);
	}

	time creation_time(int8_t zone = local_time_zone()) const {
		return from_sys_time(_data.ftCreationTime, zone);
	}

	time access_time(int8_t zone = local_time_zone()) const {
		return from_sys_time(_data.ftLastAccessTime, zone);
	}

private:
	T _data;
};

using file_info = basic_file_info<BY_HANDLE_FILE_INFORMATION>;

class file : public sys_stream {
public:
	file() : sys_stream() {}

	file(sys_stream s) : sys_stream(std::move(s)) {}

	file(native_handle_t h, bool need_close = true) :
		sys_stream(h, need_close) {}

	file_info info() const {
		file_info r;
		if (GetFileInformationByHandle(native_handle(), &r.native_data())) {
			return r;
		}
		memset(&r, 0, sizeof(r));
		return r;
	}

	bool is_dir() const {
		return info().is_dir();
	}

	uint64_t size() const {
		LARGE_INTEGER sz;
		if (!GetFileSizeEx(native_handle(), &sz)) {
			return 0;
		}
		return static_cast<uint64_t>(sz.QuadPart);
	}

	virtual int64_t seek(int64_t offset, uchar whence = 0) {
		LARGE_INTEGER off_li, seeked_li;
		off_li.QuadPart =
			static_cast<decltype(LARGE_INTEGER::QuadPart)>(offset);
		if (!SetFilePointerEx(native_handle(), off_li, &seeked_li, whence)) {
			return -1;
		}
		return static_cast<int64_t>(seeked_li.QuadPart);
	}

	bytes read_all() {
		auto fsz = size();
		bytes buf(static_cast<size_t>(fsz));
		auto rsz = read_full(buf);
		if (rsz >= 0 && static_cast<uint64_t>(rsz) != fsz) {
			buf.reset();
		}
		return buf;
	}

	file_times times(int8_t zone = local_time_zone()) const {
		FILETIME lpCreationTime, lpLastAccessTime, lpLastWriteTime;
		if (!GetFileTime(
				native_handle(),
				&lpCreationTime,
				&lpLastAccessTime,
				&lpLastWriteTime)) {
			return {};
		}
		return {
			from_sys_time(lpLastWriteTime, zone),
			from_sys_time(lpCreationTime, zone),
			from_sys_time(lpLastAccessTime, zone)};
	}

	bool change_times(const file_times &times) {
		auto lpCreationTime = to_sys_time(times.creation_time);
		auto lpLastAccessTime = to_sys_time(times.access_time);
		auto lpLastWriteTime = to_sys_time(times.modified_time);
		return SetFileTime(
			native_handle(),
			&lpCreationTime,
			&lpLastAccessTime,
			&lpLastWriteTime);
	}

	time modified_time(int8_t zone = local_time_zone()) const {
		FILETIME ft;
		if (!GetFileTime(native_handle(), nullptr, nullptr, &ft)) {
			return {};
		}
		return from_sys_time(ft, zone);
	}

	bool change_modified_time(const time &t) {
		auto ft = to_sys_time(t);
		return SetFileTime(native_handle(), nullptr, nullptr, &ft);
	}

	time creation_time(int8_t zone = local_time_zone()) const {
		FILETIME ft;
		if (!GetFileTime(native_handle(), &ft, nullptr, nullptr)) {
			return {};
		}
		return from_sys_time(ft, zone);
	}

	bool change_creation_time(const time &t) {
		auto ft = to_sys_time(t);
		return SetFileTime(native_handle(), &ft, nullptr, nullptr);
	}

	time access_time(int8_t zone = local_time_zone()) const {
		FILETIME ft;
		if (!GetFileTime(native_handle(), nullptr, &ft, nullptr)) {
			return {};
		}
		return from_sys_time(ft, zone);
	}

	bool change_access_time(const time &t) {
		auto ft = to_sys_time(t);
		return SetFileTime(native_handle(), nullptr, &ft, nullptr);
	}
};

using dir_entry_info = basic_file_info<WIN32_FIND_DATAW>;

class dir_entry : public dir_entry_info {
public:
	dir_entry() = default;

	std::string name() const {
		return w_to_u8(native_data().cFileName);
	}

	file_path path() const {
		return {_dir_path, name()};
	}

	file_path relative_path() const {
		return {_dir_rel_path, name()};
	}

	const dir_entry_info &info() const {
		return *static_cast<const dir_entry_info *>(this);
	}

private:
	std::string _dir_path, _dir_rel_path;

	friend class view_dir;
};

class view_dir : private wandering_iterator {
public:
	using native_handle_t = HANDLE;

	////////////////////////////////////////////////////////////////////////

	view_dir() : _h(INVALID_HANDLE_VALUE) {}

	view_dir(const file_path &path, size_t depth = 1) :
		_entry(), _dep(depth), _parent(nullptr) {

		_entry._dir_path = path.abs().str();

		auto find_path = u8_to_w("\\\\?\\" + _entry._dir_path + "\\*");

		_h = FindFirstFileW(find_path.c_str(), &_entry.native_data());
		if (!*this) {
			return;
		}

		while (_is_dots(_entry.native_data().cFileName)) {
			if (!FindNextFileW(_h, &_entry.native_data())) {
				FindClose(_h);
				_h = INVALID_HANDLE_VALUE;
				return;
			}
		}
	}

	~view_dir() {
		if (!*this) {
			return;
		}
		FindClose(_h);
		_h = INVALID_HANDLE_VALUE;
		if (_parent) {
			delete _parent;
		}
	}

	view_dir(view_dir &&src) :
		_h(src._h),
		_entry(std::move(src._entry)),
		_dep(src._dep),
		_parent(src._parent) {
		if (src) {
			src._h = INVALID_HANDLE_VALUE;
		}
	}

	view_dir(view_dir &src) : view_dir(std::move(src)) {}

	RUA_OVERLOAD_ASSIGNMENT_R(view_dir)

	view_dir &operator=(view_dir &src) {
		return *this = std::move(src);
	}

	native_handle_t native_handle() const {
		return _h;
	}

	operator bool() const {
		return _h != INVALID_HANDLE_VALUE;
	}

	const dir_entry &operator*() const {
		return _entry;
	}

	const dir_entry *operator->() const {
		return &_entry;
	}

	view_dir &operator++() {
		assert(*this);

		if ((_dep > 1 || !_dep) && _entry.is_dir()) {
			view_dir sub(_entry.path(), _dep > 1 ? _dep - 1 : 0);
			if (sub) {
				sub._entry._dir_rel_path = _entry.relative_path().str();
				sub._parent = new view_dir(std::move(*this));
				return *this = std::move(sub);
			}
		}

		if (_next()) {
			return *this;
		}

		while (_parent) {
			auto parent = _parent;
			_parent = nullptr;
			*this = std::move(*parent);
			delete parent;

			if (_next()) {
				return *this;
			}
		}

		FindClose(_h);
		_h = INVALID_HANDLE_VALUE;
		return *this;
	}

private:
	HANDLE _h;
	dir_entry _entry;
	size_t _dep;
	view_dir *_parent;

	static bool _is_dots(const WCHAR *c_wstr) {
		for (; *c_wstr; ++c_wstr) {
			if (*c_wstr != L'.') {
				return false;
			}
		}
		return true;
	}

	bool _next() {
		while (FindNextFileW(_h, &_entry.native_data())) {
			if (_is_dots(_entry.native_data().cFileName)) {
				continue;
			}
			return true;
		}
		return false;
	}
};

namespace _make_file {

inline bool touch_dir(const file_path &path) {
	if (!path) {
		return true;
	}

	auto path_w = u8_to_w("\\\\?\\" + path.abs().str());

	auto fa = GetFileAttributesW(path_w.c_str());
	if (fa != INVALID_FILE_ATTRIBUTES && fa & FILE_ATTRIBUTE_DIRECTORY) {
		return true;
	}
	if (!touch_dir(path.rm_back())) {
		return false;
	}
	return CreateDirectoryW(path_w.c_str(), nullptr);
}

inline file make_file(const file_path &path) {
	if (!touch_dir(path.rm_back())) {
		return nullptr;
	}

	auto path_w = u8_to_w("\\\\?\\" + path.abs().str());

	return CreateFileW(
		path_w.c_str(),
		GENERIC_WRITE | GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		CREATE_ALWAYS,
		FILE_FLAG_BACKUP_SEMANTICS,
		nullptr);
}

inline file touch_file(const file_path &path) {
	if (!touch_dir(path.rm_back())) {
		return nullptr;
	}

	auto path_w = u8_to_w("\\\\?\\" + path.abs().str());

	return CreateFileW(
		path_w.c_str(),
		GENERIC_WRITE | GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_ALWAYS,
		FILE_FLAG_BACKUP_SEMANTICS,
		nullptr);
}

inline file modify_file(const file_path &path, bool stat_only = false) {
	auto path_w = u8_to_w("\\\\?\\" + path.abs().str());

	return CreateFileW(
		path_w.c_str(),
		stat_only ? FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES
				  : GENERIC_WRITE | GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		nullptr);
}

inline file view_file(const file_path &path, bool stat_only = false) {
	auto path_w = u8_to_w("\\\\?\\" + path.abs().str());

	return CreateFileW(
		path_w.c_str(),
		stat_only ? FILE_READ_ATTRIBUTES : GENERIC_READ,
		FILE_SHARE_WRITE | FILE_SHARE_DELETE | FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		nullptr);
}

inline bool remove_file(const file_path &path) {
	if (!path.is_dir()) {
		return DeleteFileW(u8_to_w("\\\\?\\" + path.abs().str()).c_str());
	}
	for (auto &ety : view_dir(path)) {
		if (!remove_file(ety.path())) {
			return false;
		}
	}
	return RemoveDirectoryW(u8_to_w("\\\\?\\" + path.abs().str()).c_str());
}

} // namespace _make_file

using namespace _make_file;

}} // namespace rua::win32

#endif
