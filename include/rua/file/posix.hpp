#ifndef _RUA_FILE_POSIX_HPP
#define _RUA_FILE_POSIX_HPP

#include "base.hpp"

#include "../chrono/now/posix.hpp"
#include "../io/sys_stream/posix.hpp"
#include "../path.hpp"
#include "../range.hpp"
#include "../string/join.hpp"
#include "../types/traits.hpp"
#include "../types/util.hpp"

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

	bool is_dir() const {
		return false;
	}

	file_path absolute() const & {
		const auto &s = str();
		if (s.size() > 0 && s[0] == '/') {
			return *this;
		}
		return _absolute(s);
	}

	file_path absolute() && {
		const auto &s = str();
		if (s.size() > 0 && s[0] == '/') {
			return std::move(*this);
		}
		return _absolute(s);
	}

private:
	static file_path _absolute(const std::string &str) {
		auto c_str = realpath(str.c_str(), nullptr);
		file_path r(c_str);
		::free(c_str);
		return r;
	}
};

namespace _wkdir {

inline file_path working_dir() {
	auto c_str = get_current_dir_name();
	file_path r(c_str);
	::free(c_str);
	return r;
}

inline bool work_at(const file_path &path) {
#ifndef NDEBUG
	auto r =
#else
	return
#endif
		!::chdir(path.str().c_str());
#ifndef NDEBUG
	assert(r);
	return r;
#endif
}

} // namespace _wkdir

using namespace _wkdir;

class file_stat {
public:
	using native_data_t = struct stat;

	////////////////////////////////////////////////////////////////////////

	file_stat() = default;

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

	file_stat stat() const {
		file_stat r;
		if (!fstat(native_handle(), &r.native_data())) {
			return r;
		}
		memset(&r, 0, sizeof(r));
		return r;
	}

	bool is_dir() const {
		return stat().is_dir();
	}

	uint64_t size() const {
		return stat().size();
	}

	bytes read_all() {
		auto fsz = size();
		bytes buf(fsz);
		auto rsz = read_full(buf);
		if (rsz != fsz) {
			buf.reset();
		}
		return buf;
	}

	file_times times(int8_t zone = local_time_zone()) const {
		return stat().times(zone);
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

namespace _make_file {

inline file make_file(const file_path &path) {
	return open(path.str().c_str(), O_CREAT | O_TRUNC | O_RDWR);
}

inline file modify_or_make_file(const file_path &path) {
	return open(path.str().c_str(), O_CREAT | O_RDWR);
}

inline file modify_file(const file_path &path, bool = false) {
	return open(path.str().c_str(), O_RDWR);
}

inline file read_file(const file_path &path, bool = false) {
	return open(path.str().c_str(), O_RDONLY);
}

} // namespace _make_file

using namespace _make_file;

using dir_entry_stat = file_stat;

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

	dir_entry_stat stat() const {
		return read_file(path()).stat();
	}

	uint64_t size() const {
		return stat().size();
	}

	file_times times(int8_t zone = local_time_zone()) const {
		return stat().times(zone);
	}

	time modified_time(int8_t zone = local_time_zone()) const {
		return stat().modified_time(zone);
	}

	time creation_time(int8_t zone = local_time_zone()) const {
		return stat().creation_time(zone);
	}

	time access_time(int8_t zone = local_time_zone()) const {
		return stat().access_time(zone);
	}

private:
	struct dirent *_data;
	std::string _dir_path, _dir_rel_path;

	friend class dir_iterator;
};

class dir_iterator : private wandering_iterator {
public:
	using native_handle_t = DIR *;

	////////////////////////////////////////////////////////////////////////

	dir_iterator() : _dir(nullptr) {}

	dir_iterator(const file_path &path, size_t depth = 1) :
		_dir(opendir(path.str().c_str())), _dep(depth), _parent(nullptr) {

		if (!_dir) {
			return;
		}

		_entry._dir_path = std::move(path).absolute().str();

		if (_next()) {
			return;
		}

		closedir(_dir);
		_dir = nullptr;
	}

	~dir_iterator() {
		if (!_dir) {
			return;
		}
		closedir(_dir);
		_dir = nullptr;
		if (_parent) {
			delete _parent;
		}
	}

	dir_iterator(dir_iterator &&src) :
		_dir(src._dir),
		_entry(std::move(src._entry)),
		_dep(src._dep),
		_parent(src._parent) {
		if (src) {
			src._dir = nullptr;
		}
	}

	dir_iterator(dir_iterator &src) : dir_iterator(std::move(src)) {}

	RUA_OVERLOAD_ASSIGNMENT_R(dir_iterator)

	dir_iterator &operator=(dir_iterator &src) {
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

	dir_iterator &operator++() {
		assert(_dir);

		if ((_dep > 1 || !_dep) && _entry.is_dir()) {
			dir_iterator sub(_entry.path(), _dep > 1 ? _dep - 1 : 0);
			if (sub) {
				sub._entry._dir_rel_path = _entry.relative_path().str();
				sub._parent = new dir_iterator(std::move(*this));
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
	dir_iterator *_parent;

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

}} // namespace rua::posix

#endif
