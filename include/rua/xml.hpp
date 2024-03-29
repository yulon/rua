#ifndef _rua_xml_hpp
#define _rua_xml_hpp

#include "range.hpp"
#include "string.hpp"
#include "util.hpp"

#include <list>
#include <map>
#include <memory>

namespace rua {

class xml_node;
using xml_node_ptr = std::shared_ptr<xml_node>;
using xml_node_list = std::list<xml_node_ptr>;
using xml_attr_map = std::map<std::string, std::string>;

enum class xml_node_type : uchar {
	element = 1,
	text = 3,
	document = 9,
};

inline xml_node_ptr make_xml_element(std::string name);
inline xml_node_ptr make_xml_text(std::string content);
inline xml_node_ptr make_xml_document();

inline xml_node_ptr parse_xml_document(string_view xml);
inline xml_node_ptr parse_xml_node(string_view xml);
inline xml_node_list parse_xml_nodes(string_view xml);

class xml_collection {
public:
	class iterator : public xml_node_list::const_iterator {
	public:
		iterator() = default;

		inline iterator &operator++();

		iterator operator++(int) {
			iterator tmp = *this;
			++(*this);
			return tmp;
		}

		iterator &operator--();

		iterator operator--(int) {
			iterator tmp = *this;
			--(*this);
			return tmp;
		}

	private:
		xml_node_list::const_iterator $end;

		iterator(
			xml_node_list::const_iterator it,
			xml_node_list::const_iterator end);

		friend xml_collection;
	};

	xml_collection() = default;

	xml_collection(const xml_node_list &nodes) : $nodes(&nodes) {}

	iterator begin() const {
		if (!$nodes) {
			return iterator();
		}
		return iterator($nodes->begin(), $nodes->end());
	}

	iterator end() const {
		if (!$nodes) {
			return iterator();
		}
		return iterator($nodes->end(), $nodes->end());
	}

	size_t size() const {
		return std::distance(begin(), end());
	}

private:
	const xml_node_list *$nodes;
};

class xml_node : public std::enable_shared_from_this<xml_node> {
public:
	virtual ~xml_node() {}

	virtual xml_node_type node_type() const = 0;

	virtual string_view node_name() const = 0;

	virtual xml_node_ptr parent() const {
		return nullptr;
	}

	virtual bool remove() {
		return false;
	}

	virtual const xml_node_list &child_nodes() const {
		static xml_node_list empty;
		return empty;
	}

	xml_collection children() const {
		return child_nodes();
	}

	xml_node_list
	querys(string_view selector, size_t depth = 0, size_t max_count = 0) const {
		xml_node_list matcheds;

		for (auto &node : children()) {
			if (node->node_name() == selector) {
				matcheds.emplace_back(node);
				if (max_count && matcheds.size() == max_count) {
					return matcheds;
				}
			}

			if (depth == 1) {
				continue;
			}

			auto child_matcheds = node->querys(
				selector,
				depth ? depth - 1 : 0,
				max_count ? max_count - matcheds.size() : 0);
			if (child_matcheds.size()) {
				matcheds.splice(matcheds.end(), std::move(child_matcheds));
				if (max_count && matcheds.size() == max_count) {
					return matcheds;
				}
			}
		}

		return matcheds;
	}

	xml_node_ptr query(string_view selector, size_t depth = 0) const {
		auto matcheds = querys(selector, depth, 1);
		if (matcheds.empty()) {
			return nullptr;
		}
		return matcheds.front();
	}

	bool is_contain(const xml_node_ptr &node) const {
		return $is_contain(node.get());
	}

	bool is_adoptable(const xml_node_ptr &node) const {
		if (!node || node.get() == this || node->$is_contain(this)) {
			return false;
		}
		auto p = parent();
		if (p) {
			return p->is_adoptable(node);
		}
		return true;
	}

	virtual bool append_child(
		xml_node_ptr /* node */, bool /* check_circular_ref */ = true) {
		return false;
	}

	template <
		typename NodeList,
		typename = enable_if_t<std::is_same<
			typename range_traits<NodeList &&>::value_type,
			xml_node_ptr>::value>>
	size_t append_child(NodeList &&nodes, bool check_circular_ref = true) {
		auto c = 0;
		for (auto node : std::forward<NodeList>(nodes)) {
			if (append_child(std::move(node), check_circular_ref)) {
				++c;
			}
		}
		return c;
	}

	bool append(xml_node_ptr node, bool check_circular_ref = true) {
		if (!node) {
			node = make_xml_text("null");
		}
		return append_child(std::move(node), check_circular_ref);
	}

	bool append(std::string str, bool check_circular_ref = true) {
		return append_child(make_xml_text(std::move(str)), check_circular_ref);
	}

	bool append(string_view str, bool check_circular_ref = true) {
		return append_child(make_xml_text(to_string(str)), check_circular_ref);
	}

	template <typename NodeList>
	enable_if_t<
		std::is_same<
			typename range_traits<NodeList &&>::value_type,
			xml_node_ptr>::value,
		size_t>
	append(NodeList &&nodes, bool check_circular_ref = true) {
		auto c = 0;
		for (auto node : std::forward<NodeList>(nodes)) {
			if (append(std::move(node), check_circular_ref)) {
				++c;
			}
		}
		return c;
	}

	template <typename StringList>
	enable_if_t<
		std::is_same<
			typename range_traits<StringList &&>::value_type,
			std::string>::value,
		size_t>
	append(StringList &&strs, bool check_circular_ref = true) {
		auto c = 0;
		for (auto str : std::forward<StringList>(strs)) {
			if (append(std::move(str), check_circular_ref)) {
				++c;
			}
		}
		return c;
	}

	template <typename StringViewList>
	enable_if_t<
		std::is_same<
			typename range_traits<StringViewList &&>::value_type,
			string_view>::value,
		size_t>
	append(StringViewList &&strvs, bool check_circular_ref = true) {
		auto c = 0;
		for (auto strv : std::forward<StringViewList>(strvs)) {
			if (append(strv, check_circular_ref)) {
				++c;
			}
		}
		return c;
	}

	virtual bool remove_child(xml_node_ptr /* child */) {
		return false;
	}

	virtual bool replace_child(
		xml_node_ptr /* new_child */,
		xml_node_ptr /* old_child */,
		bool /* check_circular_ref */ = true) {
		return false;
	}

	bool remove_child_all() {
		auto cns = child_nodes();
		for (auto it = cns.begin(); it != cns.end();) {
			remove_child(*(it++));
		}
		assert(cns.empty());
		return true;
	}

	template <typename NodeList>
	enable_if_t<
		std::is_same<
			typename range_traits<NodeList &&>::value_type,
			xml_node_ptr>::value,
		size_t>
	replace_child_all(NodeList &&nodes, bool check_circular_ref = true) {
		remove_child_all();
		return append_child(std::forward<NodeList>(nodes), check_circular_ref);
	}

	virtual xml_attr_map &attrs() {
		static xml_attr_map empty;
		return empty;
	}

	virtual const xml_attr_map &attrs() const {
		static xml_attr_map empty;
		return empty;
	}

	virtual std::string &attr(const std::string &name) {
		return attrs()[name];
	}

	virtual std::string get_node_value() const {
		return "";
	}

	virtual bool set_node_value(std::string) {
		return false;
	}

	virtual std::string text_content() const {
		return get_node_value();
	}

	virtual std::string get_inner_text() const {
		return text_content();
	}

	virtual bool set_inner_text(std::string content) {
		return set_node_value(std::move(content));
	}

	virtual std::string get_outer_text() const {
		return get_inner_text();
	}

	bool set_outer_text(std::string content) {
		auto p = parent();
		if (!p) {
			return false;
		}
		if (content.empty()) {
			return remove();
		}
		return p->replace_child(
			make_xml_text(std::move(content)), shared_from_this(), false);
	}

	virtual std::string get_inner_xml(bool /* xml_style */ = true) const {
		return "";
	}

	virtual bool set_inner_xml(string_view) {
		return false;
	}

	virtual std::string get_outer_xml(bool /* xml_style */ = true) const {
		return "";
	}

	bool set_outer_xml(string_view xml) {
		auto p = parent();
		if (!p) {
			return false;
		}
		auto nodes = parse_xml_nodes(xml);
		if (nodes.empty()) {
			return remove();
		}
		return p->replace_child(
			std::move(nodes.front()), shared_from_this(), false);
	}

private:
	bool $is_contain(const xml_node *node) const {
		if (!node) {
			return false;
		}
		for (auto &cn : child_nodes()) {
			if (cn.get() == node || cn->$is_contain(node)) {
				return true;
			}
		}
		return false;
	}

protected:
	xml_node() = default;
};

inline xml_collection::iterator::iterator(
	xml_node_list::const_iterator it, xml_node_list::const_iterator end) :
	xml_node_list::const_iterator(std::move(it)), $end(std::move(end)) {
	while (*this != $end && (*(*this))->node_type() != xml_node_type::element) {
		xml_node_list::const_iterator::operator++();
	}
}

inline xml_collection::iterator &xml_collection::iterator::operator++() {
	do {
		xml_node_list::const_iterator::operator++();
	} while (*this != $end && (**this)->node_type() != xml_node_type::element);
	return *this;
}

inline xml_collection::iterator &xml_collection::iterator::operator--() {
	do {
		xml_node_list::const_iterator::operator--();
		if (*this == $end) {
			break;
		}
	} while (*this != $end && (**this)->node_type() != xml_node_type::element);
	return *this;
}

class xml_parent_base;

class xml_child_base : public xml_node {
public:
	virtual ~xml_child_base() {}

	virtual xml_node_ptr parent() const {
		return $parent.lock();
	}

	inline virtual bool remove();

private:
	std::weak_ptr<xml_node> $parent;
	xml_node_list::iterator $iterator;

	friend xml_parent_base;

protected:
	xml_child_base() = default;
};

class xml_parent_base : public xml_child_base {
public:
	virtual ~xml_parent_base() {}

	virtual const xml_node_list &child_nodes() const {
		return $child_nodes;
	}

	virtual bool
	append_child(xml_node_ptr node, bool check_circular_ref = true) {
		if (!node || (check_circular_ref && !is_adoptable(node))) {
			return false;
		}
		auto detailed = std::static_pointer_cast<xml_child_base>(node);
		if (detailed->$parent.use_count() && !node->remove()) {
			return false;
		}
		$child_nodes.emplace_back(std::move(node));
		detailed->$parent = RUA_WEAK_FROM_THIS;
		detailed->$iterator = --$child_nodes.end();
		return true;
	}

	virtual bool remove_child(xml_node_ptr child) {
		if (!child || child->parent().get() != this) {
			return false;
		}
		auto detailed = std::static_pointer_cast<xml_child_base>(child);
		$child_nodes.erase(detailed->$iterator);
		detailed->$parent.reset();
		return true;
	}

	virtual bool replace_child(
		xml_node_ptr new_child,
		xml_node_ptr old_child,
		bool check_circular_ref = true) {

		if (!new_child || (check_circular_ref && !is_adoptable(new_child)) ||
			!old_child || old_child->parent().get() != this) {
			return false;
		}

		if (new_child == old_child) {
			return true;
		}

		auto detailed_old = std::static_pointer_cast<xml_child_base>(old_child);
		auto wk_this = std::move(detailed_old->$parent);
		auto it = std::move(detailed_old->$iterator);

		auto detailed_new = std::static_pointer_cast<xml_child_base>(new_child);
		if (detailed_new->$parent.use_count() && !new_child->remove()) {
			detailed_old->$parent = std::move(wk_this);
			return false;
		}

		*it = std::move(new_child);
		detailed_new->$parent = std::move(wk_this);
		detailed_new->$iterator = std::move(it);
		return true;
	}

	virtual std::string text_content() const {
		std::string text;
		for (auto &cn : $child_nodes) {
			text += cn->text_content();
		}
		return text;
	}

	virtual bool set_inner_text(std::string content) {
		if (content.empty() || !remove_child_all()) {
			return false;
		}
		return append_child(make_xml_text(std::move(content)));
	}

	virtual std::string get_inner_xml(bool xml_style) const {
		std::string xml;
		for (auto &cn : $child_nodes) {
			xml += cn->get_outer_xml(xml_style);
		}
		return xml;
	}

	virtual bool set_inner_xml(string_view xml) {
		auto nodes = parse_xml_nodes(xml);
		if (nodes.empty()) {
			return remove_child_all();
		}
		auto parse_sz = nodes.size();
		return replace_child_all(std::move(nodes), false) == parse_sz;
	}

private:
	xml_node_list $child_nodes;

	friend xml_child_base;

protected:
	xml_parent_base() = default;
};

inline bool xml_child_base::remove() {
	if ($parent.expired()) {
		return false;
	}
	auto detailed_parent =
		std::static_pointer_cast<xml_parent_base>($parent.lock());
	detailed_parent->$child_nodes.erase($iterator);
	$parent.reset();
	return true;
}

class xml_document : public xml_parent_base {
public:
	xml_document() = default;

	virtual ~xml_document() {}

	virtual xml_node_type node_type() const {
		return xml_node_type::document;
	}

	virtual string_view node_name() const {
		return "#document";
	}

	virtual std::string get_outer_xml(bool xml_style) const {
		return get_inner_xml(xml_style);
	}
};

class xml_element : public xml_parent_base {
public:
	xml_element(std::string name) : $name(std::move(name)) {}

	virtual ~xml_element() {}

	virtual xml_node_type node_type() const {
		return xml_node_type::element;
	}

	virtual string_view node_name() const {
		return $name;
	}

	virtual xml_attr_map &attrs() {
		return $attrs;
	}

	virtual const xml_attr_map &attrs() const {
		return $attrs;
	}

	virtual std::string get_outer_xml(bool xml_style) const {
		std::string attrs_str;
		for (auto &attr : $attrs) {
			attrs_str += join({" ", attr.first});
			if (attr.second.length() || xml_style) {
				attrs_str += join({"=\"", attr.second, "\""});
			}
		}
		if (child_nodes().empty() && xml_style) {
			return join({"<", $name, attrs_str, " />"});
		}
		return join(
			{"<",
			 $name,
			 attrs_str,
			 ">",
			 get_inner_xml(xml_style),
			 "</",
			 node_name(),
			 ">"});
	}

private:
	std::string $name;
	xml_attr_map $attrs;
};

class xml_text : public xml_child_base {
public:
	xml_text(std::string content) : $cont(std::move(content)) {}

	virtual ~xml_text() {}

	virtual xml_node_type node_type() const {
		return xml_node_type::text;
	}

	virtual string_view node_name() const {
		return "#text";
	}

	virtual std::string get_node_value() const {
		return $cont;
	}

	virtual bool set_node_value(std::string content) {
		$cont = std::move(content);
		return true;
	}

	virtual std::string get_inner_xml(bool) const {
		return $cont;
	}

	virtual bool set_inner_xml(string_view) {
		// TODO
		return false;
	}

	virtual std::string get_outer_xml(bool) const {
		return $cont;
	}

private:
	std::string $cont;
};

inline xml_node_ptr make_xml_element(std::string name) {
	return std::static_pointer_cast<xml_node>(
		std::make_shared<xml_element>(std::move(name)));
}

inline xml_node_ptr make_xml_text(std::string content) {
	return std::static_pointer_cast<xml_node>(
		std::make_shared<xml_text>(std::move(content)));
}

inline xml_node_ptr make_xml_document() {
	return std::static_pointer_cast<xml_node>(std::make_shared<xml_document>());
}

inline xml_node_ptr parse_xml_document(string_view xml) {
	auto nodes = parse_xml_nodes(xml);
	if (nodes.empty()) {
		return nullptr;
	}
	auto doc = make_xml_document();
	doc->replace_child_all(std::move(nodes));
	return doc;
}

inline xml_node_ptr parse_xml_node(string_view xml) {
	auto nodes = parse_xml_nodes(xml);
	return nodes.size() ? std::move(nodes.front()) : nullptr;
}

inline xml_node_list parse_xml_nodes(string_view xml) {
	xml_node_list nodes;
	std::list<xml_node_list::iterator> uncloseds;

	for (size_t i = 0; i < xml.length();) {
		if (xml[i] == '<') {
			++i;

			if (!skip_space(xml, i)) {
				return nodes;
			}

			enum class tag_state { closed, opening, closing } status;

			switch (xml[i]) {

			case '/':
				status = tag_state::closing;
				++i;
				break;

			case '?':
				status = tag_state::closed;
				++i;
				break;

			case '!':
				status = tag_state::closed;
				++i;
				if (xml.length() - i >= c_str_len("---->") &&
					string_view(&xml[i], 2) == "--") {
					do {
						++i;
						if (i + 2 >= xml.length()) {
							return nodes;
						}
					} while (string_view(&xml[i], 3) != "-->");
					continue;
				}
				break;

			default:
				status = tag_state::opening;
			}

			if (!skip_space(xml, i)) {
				return nodes;
			}

			auto node_name_start = i;
			do {
				++i;
				if (i >= xml.length()) {
					return nodes;
				}
			} while (!is_unispace(xml[i]) && xml[i] != '>' && xml[i] != '/' &&
					 xml[i] != '?');

			auto name =
				to_string(xml.substr(node_name_start, i - node_name_start));

			if (status < tag_state::closing) {
				auto node = make_xml_element(std::move(name));

				for (;;) {
					if (!skip_space(xml, i)) {
						return nodes;
					}

					if (xml[i] == '/' || xml[i] == '?') {
						status = tag_state::closed;
						++i;
					}

					if (!skip_space(xml, i)) {
						return nodes;
					}

					if (xml[i] == '>') {
						++i;
						break;
					}

					auto attr_name_start = i;
					do {
						++i;
						if (i >= xml.length()) {
							return nodes;
						}
					} while (!is_unispace(xml[i]) && xml[i] != '=' &&
							 xml[i] != '>' && xml[i] != '/' && xml[i] != '?');

					auto attr_name = to_string(
						xml.substr(attr_name_start, i - attr_name_start));

					if (!skip_space(xml, i)) {
						return nodes;
					}

					if (xml[i] != '=') {
						node->attrs()[attr_name];
						continue;
					}

					do {
						++i;
						if (i >= xml.length()) {
							return nodes;
						}
					} while (xml[i] != '\"');
					++i;

					auto attr_val_start = i;
					while (xml[i] != '\"') {
						++i;
						if (i >= xml.length()) {
							return nodes;
						}
					}

					node->attrs()[attr_name] = to_string(
						xml.substr(attr_val_start, i - attr_val_start));

					++i;
				}

				nodes.emplace_back(std::move(node));

				if (status == tag_state::opening) {
					uncloseds.emplace_front(--nodes.end());
				}

				continue;
			}

			while (xml[i] != '>') {
				++i;
				if (i >= xml.length()) {
					return nodes;
				}
			}
			++i;

			for (auto itit = uncloseds.begin(); itit != uncloseds.end();
				 ++itit) {
				auto &it = *itit;
				auto &n = *it;
				if (n->node_type() != xml_node_type::element ||
					n->node_name() != name) {
					continue;
				}
				xml_node_list cns;
				cns.splice(cns.end(), nodes, ++it, nodes.end());
				n->append_child(std::move(cns), false);
				uncloseds.erase(uncloseds.begin(), ++itit);
				break;
			}
			continue;
		}

		auto text_start = i;
		do {
			++i;
		} while (i < xml.length() && xml[i] != '<');

		auto text = xml.substr(text_start, i - text_start);
		if (is_unispaces(text)) {
			continue;
		}

		nodes.emplace_back(
			make_xml_text(to_string(xml.substr(text_start, i - text_start))));
	}

	return nodes;
}

} // namespace rua

#endif
