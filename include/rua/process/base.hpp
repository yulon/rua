#ifndef _rua_process_base_hpp
#define _rua_process_base_hpp

#include "../optional.hpp"
#include "../range/traits.hpp"
#include "../string/conv.hpp"
#include "../util.hpp"

#include <functional>
#include <list>
#include <map>

namespace rua {

template <typename Process, typename FilePath, typename StdioStream>
struct _baisc_process_make_info {
	using process_t = Process;
	using file_path_t = FilePath;
	using stdio_stream_t = StdioStream;

	FilePath file, work_dir;
	std::list<std::string> args;
	optional<StdioStream> stdout_w, stderr_w, stdin_r;
	std::map<std::string, std::string> envs;
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

	RUA_OVERLOAD_ASSIGNMENT(_baisc_process_make_info);*/
};

template <typename ProcessMaker, typename ProcessMakeInfo>
class process_maker_base {
public:
	using process_t = typename ProcessMakeInfo::process_t;
	using file_path_t = typename ProcessMakeInfo::file_path_t;
	using stdio_stream_t = typename ProcessMakeInfo::stdio_stream_t;

	process_maker_base() = default;

	/*process_maker_base(process_maker_base &&pmb) :
		$info(std::move(pmb.$info)) {}

	RUA_OVERLOAD_ASSIGNMENT(process_maker_base);*/

	template <typename Arg>
	ProcessMaker &arg(Arg &&a) & {
		$info.args.emplace_back(to_string(std::forward<Arg>(a)));
		return *$this();
	}

	template <typename Arg>
	ProcessMaker &&arg(Arg &&a) && {
		return std::move(arg(std::forward<Arg>(a)));
	}

	ProcessMaker &args(std::list<std::string> a_str_li) & {
		if (!a_str_li.size()) {
			return *$this();
		}
		if ($info.args.size()) {
			$info.args.splice($info.args.end(), std::move(a_str_li));
			return *$this();
		}
		$info.args = std::move(a_str_li);
		return *$this();
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
		for (auto &a_str : a_str_li) {
			backs.emplace_back(to_string(std::move(a_str)));
		}
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
		return *$this();
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
		$info.work_dir = std::move(work_dir);
		return *$this();
	}

	ProcessMaker &&work_at(file_path_t work_dir) && {
		return std::move(work_at(std::move(work_dir)));
	}

	ProcessMaker &env(std::string name, std::string val) & {
		$info.envs[std::move(name)] = std::move(val);
		return *$this();
	}

	ProcessMaker &&env(std::string name, std::string val) && {
		return std::move(env(std::move(name), std::move(val)));
	}

	ProcessMaker &envs(std::map<std::string, std::string> env_map) & {
		$info.envs = std::move(env_map);
		return *$this();
	}

	ProcessMaker &&envs(std::map<std::string, std::string> env_map) && {
		return std::move(envs(std::move(env_map)));
	}

	ProcessMaker &hide() & {
		$info.hide = true;
		return *$this();
	}

	ProcessMaker &&hide() && {
		return std::move(hide());
	}

	ProcessMaker &stdout_to(stdio_stream_t stdout_w) & {
		$info.stdout_w = std::move(stdout_w);
		return *$this();
	}

	ProcessMaker &&stdout_to(stdio_stream_t stdout_w) && {
		return std::move(stdout_to(std::move(stdout_w)));
	}

	ProcessMaker &stderr_to(stdio_stream_t stderr_w) & {
		$info.stderr_w = std::move(stderr_w);
		return *$this();
	}

	ProcessMaker &&stderr_to(stdio_stream_t stderr_w) && {
		return std::move(stderr_to(std::move(stderr_w)));
	}

	ProcessMaker &stdin_from(stdio_stream_t stdin_r) & {
		$info.stdin_r = std::move(stdin_r);
		return *$this();
	}

	ProcessMaker &&stdin_from(stdio_stream_t stdin_r) && {
		return std::move(stdin_from(std::move(stdin_r)));
	}

	ProcessMaker &on_start(std::function<void(process_t &)> cb) & {
		$info.on_start = std::move(cb);
		return *$this();
	}

	ProcessMaker &&on_start(std::function<void(process_t &)> cb) && {
		return std::move(on_start(std::move(cb)));
	}

	ProcessMaker &load_dylib(std::string name) & {
		$info.dylibs.push_back(std::move(name));
		return *$this();
	}

	ProcessMaker &&load_dylib(std::string name) && {
		return std::move(load_dylib(std::move(name)));
	}

protected:
	ProcessMakeInfo $info;

	process_maker_base(ProcessMakeInfo info) : $info(std::move(info)) {}

private:
	ProcessMaker *$this() {
		return static_cast<ProcessMaker *>(this);
	}
};

} // namespace rua

#endif
