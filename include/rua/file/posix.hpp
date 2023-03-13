#ifndef _rua_file_posix_hpp
#define _rua_file_posix_hpp

#include "times.hpp"

#include "../path.hpp"
#include "../range.hpp"
#include "../string/join.hpp"
#include "../sys/stream/posix.hpp"
#include "../time/now/posix.hpp"
#include "../util.hpp"

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <string>

namespace rua { namespace posix {

class file_path : public path_base<file_path> {
public:
	RUA_PATH_CTORS(file_path)

	bool is_exist() const {
		struct stat st;
		return stat(str().c_str(), &st) == 0;
	}

	bool is_dir() const {
		struct stat st;
		return stat(str().c_str(), &st) == 0 ? S_ISDIR(st.st_mode) : false;
	}

	file_path abs() const & {
		const auto &s = str();
		if (s.size() > 0 && s[0] == '/') {
			return *this;
		}
		return _abs(s);
	}

	file_path abs() && {
		const auto &s = str();
		if (s.size() > 0 && s[0] == '/') {
			return std::move(*this);
		}
		return _abs(s);
	}

private:
	static file_path _abs(const std::string &str) {
		auto c_str = realpath(str.c_str(), nullptr);
		file_path r(c_str);
		::free(c_str);
		return r;
	}
};

class file_info {
public:
	using native_data_t = struct stat;

	////////////////////////////////////////////////////////////////////////

	file_info() = default;

	native_data_t &native_data() {
		return _data;
	}

	const native_data_t &native_data() const {
		return _data;
	}

	bool is_dir() const {
		return S_ISDIR(_data.st_mode);
	}

	uint64_t size() const {
		return static_cast<uint64_t>(_data.st_size);
	}

	file_times times(int8_t zone = local_time_zone()) const {
		return {modified_time(zone), creation_time(zone), access_time(zone)};
	}

	time modified_time(int8_t zone = local_time_zone()) const {
		return time(_data.st_mtime, zone, unix_epoch);
	}

	time creation_time(int8_t zone = local_time_zone()) const {
		return time(_data.st_ctime, zone, unix_epoch);
	}

	time access_time(int8_t zone = local_time_zone()) const {
		return time(_data.st_atime, zone, unix_epoch);
	}

private:
	struct stat _data;
};

class file : public sys_stream {
public:
	file() : sys_stream() {}

	file(sys_stream s) : sys_stream(std::move(s)) {}

	file(native_handle_t fd, bool need_close = true) :
		sys_stream(fd, need_close) {}

	template <
		typename NullPtr,
		typename = enable_if_t<is_null_pointer<NullPtr>::value>>
	constexpr file(NullPtr) : sys_stream() {}

	file_info info() const {
		file_info r;
		if (!fstat(native_handle(), &r.native_data())) {
			return r;
		}
		memset(&r, 0, sizeof(r));
		return r;
	}

	bool is_dir() const {
		return info().is_dir();
	}

	uint64_t size() const {
		return info().size();
	}

	virtual int64_t seek(int64_t offset, uchar whence = 0) {
		return static_cast<int64_t>(lseek(native_handle(), offset, whence));
	}

	bytes read_all() {
		auto fsz = size();
		bytes buf(fsz);
		auto rsz = read_full(buf);
		if (rsz >= 0 && static_cast<uint64_t>(rsz) != fsz) {
			buf.reset();
		}
		return buf;
	}

	file_times times(int8_t zone = local_time_zone()) const {
		return info().times(zone);
	}

	time modified_time(int8_t zone = local_time_zone()) const {
		return times(zone).modified_time;
	}

	time creation_time(int8_t zone = local_time_zone()) const {
		return times(zone).creation_time;
	}

	time access_time(int8_t zone = local_time_zone()) const {
		return times(zone).access_time;
	}
};

using dir_entry_info = file_info;

class dir_entry {
public:
	using native_data_t = struct dirent;

	////////////////////////////////////////////////////////////////////////

	dir_entry() = default;

	native_data_t &native_data() {
		return *_data;
	}

	const native_data_t &native_data() const {
		return *_data;
	}

	std::string name() const {
		return _data->d_name;
	}

	file_path path() const {
		return {_dir_path, name()};
	}

	file_path relative_path() const {
		return {_dir_rel_path, name()};
	}

	bool is_dir() const {
		return _data->d_type == DT_DIR;
	}

	dir_entry_info info() const {
		dir_entry_info r;
		if (!stat(path().str().c_str(), &r.native_data())) {
			return r;
		}
		memset(&r, 0, sizeof(r));
		return r;
	}

	uint64_t size() const {
		return info().size();
	}

	file_times times(int8_t zone = local_time_zone()) const {
		return info().times(zone);
	}

	time modified_time(int8_t zone = local_time_zone()) const {
		return info().modified_time(zone);
	}

	time creation_time(int8_t zone = local_time_zone()) const {
		return info().creation_time(zone);
	}

	time access_time(int8_t zone = local_time_zone()) const {
		return info().access_time(zone);
	}

private:
	struct dirent *_data;
	std::string _dir_path, _dir_rel_path;

	friend class view_dir;
};

class view_dir : private wandering_iterator {
public:
	using native_handle_t = DIR *;

	////////////////////////////////////////////////////////////////////////

	view_dir() : _dir(nullptr) {}

	view_dir(const file_path &path, size_t depth = 1) :
		_dir(opendir(path.str().c_str())), _dep(depth), _parent(nullptr) {

		if (!_dir) {
			return;
		}

		_entry._dir_path = std::move(path).abs().str();

		if (_next()) {
			return;
		}

		closedir(_dir);
		_dir = nullptr;
	}

	~view_dir() {
		if (!_dir) {
			return;
		}
		closedir(_dir);
		_dir = nullptr;
		if (_parent) {
			delete _parent;
		}
	}

	view_dir(view_dir &&src) :
		_dir(src._dir),
		_entry(std::move(src._entry)),
		_dep(src._dep),
		_parent(src._parent) {
		if (src) {
			src._dir = nullptr;
		}
	}

	view_dir(view_dir &src) : view_dir(std::move(src)) {}

	RUA_OVERLOAD_ASSIGNMENT(view_dir)

	view_dir &operator=(view_dir &src) {
		return *this = std::move(src);
	}

	native_handle_t native_handle() const {
		return _dir;
	}

	operator bool() const {
		return _dir;
	}

	const dir_entry &operator*() const {
		return _entry;
	}

	const dir_entry *operator->() const {
		return &_entry;
	}

	view_dir &operator++() {
		assert(_dir);

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

		closedir(_dir);
		_dir = nullptr;
		return *this;
	}

private:
	DIR *_dir;
	dir_entry _entry;
	size_t _dep;
	view_dir *_parent;

	static bool _is_dots(const char *c_str) {
		for (; *c_str; ++c_str) {
			if (*c_str != '.') {
				return false;
			}
		}
		return true;
	}

	bool _next() {
		while (assign(_entry._data, readdir(_dir))) {
			if (_is_dots(_entry._data->d_name)) {
				continue;
			}
			return true;
		}
		return false;
	}
};

namespace _make_file {

inline bool touch_dir(const file_path &path, mode_t mode = 0777) {
	if (!path || path.is_dir()) {
		return true;
	}
	if (!touch_dir(path.rm_back(), mode)) {
		return false;
	}
	return mkdir(path.str().c_str(), mode) == 0;
}

inline file make_file(const file_path &path) {
	if (!touch_dir(path.rm_back())) {
		return nullptr;
	}
	return open(path.str().c_str(), O_CREAT | O_TRUNC | O_RDWR);
}

inline file touch_file(const file_path &path) {
	if (!touch_dir(path.rm_back())) {
		return nullptr;
	}
	return open(path.str().c_str(), O_CREAT | O_RDWR);
}

inline file modify_file(const file_path &path, bool = false) {
	return open(path.str().c_str(), O_RDWR);
}

inline file view_file(const file_path &path, bool = false) {
	return open(path.str().c_str(), O_RDONLY);
}

inline bool remove_file(const file_path &path) {
	if (!path.is_dir()) {
		return !unlink(path.str().c_str());
	}
	for (auto &ety : view_dir(path)) {
		remove_file(ety.path());
	}
	return !rmdir(path.str().c_str());
}

inline bool copy_file(
	const file_path &src, const file_path &dest, bool replaceable_dest = true) {
	if (!replaceable_dest && dest.is_exist()) {
		return false;
	}
	auto dest_f = make_file(dest);
	if (!dest_f) {
		return false;
	}
	auto src_f = view_file(src);
	if (!src_f) {
		return false;
	}
	return dest_f.copy(src_f);
}

inline bool move_file(
	const file_path &src, const file_path &dest, bool replaceable_dest = true) {
	if (dest.is_exist()) {
		if (!replaceable_dest) {
			return false;
		}
		return remove_file(dest);
	}
	auto err = ::rename(src.str().c_str(), dest.str().c_str());
	if (!err) {
		return true;
	}
	auto ok = copy_file(src, dest, replaceable_dest);
	if (!ok) {
		return false;
	}
	return remove_file(src);
}

} // namespace _make_file

using namespace _make_file;

}} // namespace rua::posix

#endif
