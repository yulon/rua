#ifndef _RUA_PROCESS_BASE_HPP
#define _RUA_PROCESS_BASE_HPP

#include "../any_word.hpp"
#include "../optional.hpp"
#include "../range.hpp"
#include "../string/conv.hpp"
#include "../types/traits.hpp"

#include <functional>
#include <list>

namespace rua {

template <typename Process, typename FilePath, typename StdioStream>
struct _baisc_process_make_info {
	using process_t = Process;
	using file_path_t = FilePath;
	using stdio_stream_t = StdioStream;

	FilePath file, work_dir;
	std::list<std::string> args;
	optional<StdioStream> stdout_w, stderr_w, stdin_r;
	bool hide;
	std::function<void(process_t &)> on_start;
	std::list<std::string> dylibs;

	_baisc_process_make_info() = default;

	_baisc_process_make_info(file_path_t file) :
		file(std::move(file)), hide(false) {}

	/*_baisc_process_make_info(_baisc_process_make_info &&bpmi) :
		file(std::move(bpmi.file)),
		work_dir(std::move(bpmi.work_dir)),
		args(std::move(bpmi.args)),
		stdout_w(std::move(bpmi.stdout_w)),
		stderr_w(std::move(bpmi.stderr_w)),
		stdin_r(std::move(bpmi.stdin_r)),
		hide(bpmi.hide),
		on_start(std::move(bpmi.on_start)),
		dylibs(std::move(bpmi.dylibs)) {}

	RUA_OVERLOAD_ASSIGNMENT_R(_baisc_process_make_info);*/
};

template <typename ProcessMaker, typename ProcessMakeInfo>
class process_maker_base {
public:
	using process_t = typename ProcessMakeInfo::process_t;
	using file_path_t = typename ProcessMakeInfo::file_path_t;
	using stdio_stream_t = typename ProcessMakeInfo::stdio_stream_t;

	process_maker_base() = default;

	/*process_maker_base(process_maker_base &&pmb) :
		_info(std::move(pmb._info)) {}

	RUA_OVERLOAD_ASSIGNMENT_R(process_maker_base);*/

	template <typename Arg>
	ProcessMaker &arg(Arg &&a) & {
		_info.args.emplace_back(to_string(std::forward<Arg>(a)));
		return *_this();
	}

	template <typename Arg>
	ProcessMaker &&arg(Arg &&a) && {
		return std::move(arg(std::forward<Arg>(a)));
	}

	ProcessMaker &args(std::list<std::string> a_str_li) & {
		if (!a_str_li.size()) {
			return *_this();
		}
		if (_info.args.size()) {
			_info.args.splice(_info.args.end(), std::move(a_str_li));
			return *_this();
		}
		_info.args = std::move(a_str_li);
		return *_this();
	}

	ProcessMaker &&args(std::list<std::string> a_str_li) && {
		return std::move(args(a_str_li));
	}

	template <RUA_STRING_RANGE(StrList)>
	enable_if_t<
		!std::is_base_of<std::list<std::string>, decay_t<StrList>>::value,
		ProcessMaker &>
	args(StrList &&a_str_li) & {
		std::list<std::string> backs;
		RUA_RANGE_FOR(auto &a_str, a_str_li, {
			backs.emplace_back(to_string(std::move(a_str)));
		})
		return args(std::move(backs));
	}

	template <RUA_STRING_RANGE(StrList)>
	enable_if_t<
		!std::is_base_of<std::list<std::string>, decay_t<StrList>>::value,
		ProcessMaker &&>
	args(StrList &&a_str_li) && {
		return std::move(args(std::forward<StrList>(a_str_li)));
	}

	ProcessMaker &args() & {
		return *_this();
	}

	template <typename OneArgs>
	enable_if_t<
		std::is_convertible<OneArgs &&, string_view>::value,
		ProcessMaker &>
	args(OneArgs &&a) & {
		return arg(std::forward<OneArgs>(a));
	}

	template <typename... Args>
	enable_if_t<(sizeof...(Args) > 1), ProcessMaker &> args(Args &&...a) & {
		return args(
			std::list<std::string>({to_string(std::forward<Args>(a))...}));
	}

	template <typename... Args>
	enable_if_t<
		std::is_convertible<front_t<Args &&...>, string_view>::value ||
			(sizeof...(Args) > 1),
		ProcessMaker &&>
	args(Args &&...a) && {
		return std::move(args(std::forward<Args>(a)...));
	}

	ProcessMaker &work_at(file_path_t work_dir) & {
		_info.work_dir = std::move(work_dir);
		return *_this();
	}

	ProcessMaker &&work_at(file_path_t work_dir) && {
		return std::move(work_at(std::move(work_dir)));
	}

	ProcessMaker &hide() & {
		_info.hide = true;
		return *_this();
	}

	ProcessMaker &&hide() && {
		return std::move(hide());
	}

	ProcessMaker &stdout_to(stdio_stream_t stdout_w) & {
		_info.stdout_w = std::move(stdout_w);
		return *_this();
	}

	ProcessMaker &&stdout_to(stdio_stream_t stdout_w) && {
		return std::move(stdout_to(std::move(stdout_w)));
	}

	ProcessMaker &stderr_to(stdio_stream_t stderr_w) & {
		_info.stderr_w = std::move(stderr_w);
		return *_this();
	}

	ProcessMaker &&stderr_to(stdio_stream_t stderr_w) && {
		return std::move(stderr_to(std::move(stderr_w)));
	}

	ProcessMaker &stdin_from(stdio_stream_t stdin_r) & {
		_info.stdin_r = std::move(stdin_r);
		return *_this();
	}

	ProcessMaker &&stdin_from(stdio_stream_t stdin_r) && {
		return std::move(stdin_from(std::move(stdin_r)));
	}

	ProcessMaker &on_start(std::function<void(process_t &)> cb) & {
		_info.on_start = std::move(cb);
		return *_this();
	}

	ProcessMaker &&on_start(std::function<void(process_t &)> cb) && {
		return std::move(on_start(std::move(cb)));
	}

	ProcessMaker &load_dylib(std::string name) & {
		_info.dylibs.push_back(std::move(name));
		return *_this();
	}

	ProcessMaker &&load_dylib(std::string name) && {
		return std::move(load_dylib(std::move(name)));
	}

	any_word run() {
		return _this()->start().wait();
	}

protected:
	ProcessMakeInfo _info;

	process_maker_base(ProcessMakeInfo info) : _info(std::move(info)) {}

private:
	ProcessMaker *_this() {
		return static_cast<ProcessMaker *>(this);
	}
};

} // namespace rua

#endif
