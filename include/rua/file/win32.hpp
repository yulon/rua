#ifndef _rua_file_win32_hpp
#define _rua_file_win32_hpp

#include "times.hpp"

#include "../path.hpp"
#include "../range.hpp"
#include "../span.hpp"
#include "../string/codec/base/win32.hpp"
#include "../string/join.hpp"
#include "../string/with.hpp"
#include "../sys/stream/win32.hpp"
#include "../time/now/win32.hpp"
#include "../util.hpp"

#include <windows.h>

#include <cassert>
#include <cstring>
#include <string>

namespace rua { namespace win32 {

class file_path : public path_base<file_path, '\\'> {
public:
	RUA_PATH_CTORS(file_path)

	bool is_exist() const {
		auto path_w = u2w("\\\\?\\" + abs().str());

		return GetFileAttributesW(path_w.c_str()) != INVALID_FILE_ATTRIBUTES;
	}

	bool is_dir() const {
		auto path_w = u2w("\\\\?\\" + abs().str());

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
		auto rel_w = u2w(s);
		auto abs_w_len = GetFullPathNameW(rel_w.c_str(), 0, nullptr, nullptr);
		if (!abs_w_len) {
			return *this;
		}
		return $abs(rel_w.c_str(), abs_w_len);
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
		auto rel_w = u2w(s);
		auto abs_w_len = GetFullPathNameW(rel_w.c_str(), 0, nullptr, nullptr);
		if (!abs_w_len) {
			return std::move(*this);
		}
		return $abs(rel_w.c_str(), abs_w_len);
	}

private:
	static std::string $abs(const WCHAR *rel_c_wstr, size_t abs_w_len) {
		auto buf_sz = abs_w_len + 1;
		auto abs_c_wstr = new WCHAR[buf_sz];

		abs_w_len = GetFullPathNameW(
			rel_c_wstr, static_cast<DWORD>(abs_w_len), abs_c_wstr, nullptr);

		auto r = w2u(wstring_view(abs_c_wstr, abs_w_len));
		delete[] abs_c_wstr;
		return r;
	}
};

template <typename GetDirW>
inline std::wstring _get_dir_w(GetDirW &&get_dir_w) {
	std::wstring buf;
	DWORD n;
	do {
		n = std::forward<GetDirW>(get_dir_w)(0, nullptr);
		if (!n) {
			return L"";
		}
		buf.resize(n + 1, L'\0');
		n = std::forward<GetDirW>(get_dir_w)(
			static_cast<DWORD>(buf.length()), data(buf));
	} while (!n);
	buf.resize(n);
	return buf;
}

template <typename ConvDirW>
inline std::wstring _get_dir_w(ConvDirW &&conv_dir_w, std::wstring &&src) {
	std::wstring buf;
	DWORD n;
	do {
		n = std::forward<ConvDirW>(conv_dir_w)(src.c_str(), nullptr, 0);
		if (!n) {
			return L"";
		}
		buf.resize(n + 1, L'\0');
		n = std::forward<ConvDirW>(conv_dir_w)(
			src.c_str(), data(buf), static_cast<DWORD>(buf.length()));
	} while (!n);
	buf.resize(n);
	return buf;
}

template <typename T>
class basic_file_info {
public:
	using native_data_t = T;

	////////////////////////////////////////////////////////////////////////

	basic_file_info() = default;

	native_data_t &native_data() {
		return $data;
	}

	const native_data_t &native_data() const {
		return $data;
	}

	bool is_dir() const {
		return $data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
	}

	uint64_t size() const {
		return static_cast<uint64_t>($data.nFileSizeHigh) << 32 |
			   static_cast<uint64_t>($data.nFileSizeLow);
	}

	file_times times(int8_t zone = local_time_zone()) const {
		return {modified_time(zone), creation_time(zone), access_time(zone)};
	}

	time modified_time(int8_t zone = local_time_zone()) const {
		return from_sys_time($data.ftLastWriteTime, zone);
	}

	time creation_time(int8_t zone = local_time_zone()) const {
		return from_sys_time($data.ftCreationTime, zone);
	}

	time access_time(int8_t zone = local_time_zone()) const {
		return from_sys_time($data.ftLastAccessTime, zone);
	}

private:
	T $data;
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
		return w2u(native_data().cFileName);
	}

	file_path path() const {
		return {$dir_path, name()};
	}

	file_path relative_path() const {
		return {$dir_rel_path, name()};
	}

	const dir_entry_info &info() const {
		return *static_cast<const dir_entry_info *>(this);
	}

private:
	std::string $dir_path, $dir_rel_path;

	friend class view_dir;
};

class view_dir : private wandering_iterator {
public:
	using native_handle_t = HANDLE;

	////////////////////////////////////////////////////////////////////////

	view_dir() : $h(INVALID_HANDLE_VALUE) {}

	view_dir(const file_path &path, size_t depth = 1) :
		$entry(), $dep(depth), $parent(nullptr) {

		$entry.$dir_path = path.abs().str();

		auto find_path = u2w("\\\\?\\" + $entry.$dir_path + "\\*");

		$h = FindFirstFileW(find_path.c_str(), &$entry.native_data());
		if (!*this) {
			return;
		}

		while ($is_dots($entry.native_data().cFileName)) {
			if (!FindNextFileW($h, &$entry.native_data())) {
				FindClose($h);
				$h = INVALID_HANDLE_VALUE;
				return;
			}
		}
	}

	~view_dir() {
		if (!*this) {
			return;
		}
		FindClose($h);
		$h = INVALID_HANDLE_VALUE;
		if ($parent) {
			delete $parent;
		}
	}

	view_dir(view_dir &&src) :
		$h(src.$h),
		$entry(std::move(src.$entry)),
		$dep(src.$dep),
		$parent(src.$parent) {
		if (src) {
			src.$h = INVALID_HANDLE_VALUE;
		}
	}

	view_dir(view_dir &src) : view_dir(std::move(src)) {}

	RUA_OVERLOAD_ASSIGNMENT(view_dir)

	view_dir &operator=(view_dir &src) {
		return *this = std::move(src);
	}

	native_handle_t native_handle() const {
		return $h;
	}

	operator bool() const {
		return $h != INVALID_HANDLE_VALUE;
	}

	const dir_entry &operator*() const {
		return $entry;
	}

	const dir_entry *operator->() const {
		return &$entry;
	}

	view_dir &operator++() {
		assert(*this);

		if (($dep > 1 || !$dep) && $entry.is_dir()) {
			view_dir sub($entry.path(), $dep > 1 ? $dep - 1 : 0);
			if (sub) {
				sub.$entry.$dir_rel_path = $entry.relative_path().str();
				sub.$parent = new view_dir(std::move(*this));
				return *this = std::move(sub);
			}
		}

		if ($next()) {
			return *this;
		}

		while ($parent) {
			auto parent = $parent;
			$parent = nullptr;
			*this = std::move(*parent);
			delete parent;

			if ($next()) {
				return *this;
			}
		}

		FindClose($h);
		$h = INVALID_HANDLE_VALUE;
		return *this;
	}

private:
	HANDLE $h;
	dir_entry $entry;
	size_t $dep;
	view_dir *$parent;

	static bool $is_dots(const WCHAR *c_wstr) {
		for (; *c_wstr; ++c_wstr) {
			if (*c_wstr != L'.') {
				return false;
			}
		}
		return true;
	}

	bool $next() {
		while (FindNextFileW($h, &$entry.native_data())) {
			if ($is_dots($entry.native_data().cFileName)) {
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

	auto path_w = u2w("\\\\?\\" + path.abs().str());

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

	auto path_w = u2w("\\\\?\\" + path.abs().str());

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

	auto path_w = u2w("\\\\?\\" + path.abs().str());

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
	auto path_w = u2w("\\\\?\\" + path.abs().str());

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
	auto path_w = u2w("\\\\?\\" + path.abs().str());

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
		return DeleteFileW(u2w("\\\\?\\" + path.abs().str()).c_str());
	}
	for (auto &ety : view_dir(path)) {
		remove_file(ety.path());
	}
	return RemoveDirectoryW(u2w("\\\\?\\" + path.abs().str()).c_str());
}

inline bool copy_file(
	const file_path &src, const file_path &dest, bool replaceable_dest = true) {
	auto src_w = u2w("\\\\?\\" + src.abs().str());
	auto dest_w = u2w("\\\\?\\" + dest.abs().str());

	if (!touch_dir(dest.rm_back())) {
		return false;
	}
	return CopyFileW(src_w.c_str(), dest_w.c_str(), !replaceable_dest);
}

inline bool move_file(
	const file_path &src, const file_path &dest, bool replaceable_dest = true) {
	auto src_w = u2w("\\\\?\\" + src.abs().str());
	auto dest_w = u2w("\\\\?\\" + dest.abs().str());

	if (!touch_dir(dest.rm_back())) {
		return false;
	}
	DWORD flags = MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH;
	if (replaceable_dest) {
		flags |= MOVEFILE_REPLACE_EXISTING;
	}
	return MoveFileExW(src_w.c_str(), dest_w.c_str(), flags);
}

inline bool make_link(const file_path &src, const file_path &link) {
	auto src_w = u2w("\\\\?\\" + src.abs().str());
	auto link_w = u2w("\\\\?\\" + link.abs().str());

	if (!touch_dir(link.rm_back())) {
		return false;
	}
	return CreateSymbolicLinkW(
		link_w.c_str(),
		src_w.c_str(),
		src.is_dir() ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0);
}

} // namespace _make_file

using namespace _make_file;

}} // namespace rua::win32

#endif
