#ifndef _RUA_XML_HPP
#define _RUA_XML_HPP

#include "interface_ptr.hpp"
#include "string.hpp"
#include "types/util.hpp"

#include <list>
#include <map>

namespace rua {

class _xml_node;

using _xml_node_i = interface_ptr<_xml_node>;

using _xml_attribute_map = std::map<std::string, std::string>;

using _xml_node_list = std::list<_xml_node_i>;

inline _xml_node_list _parse_xml_nodes(string_view xml);

class _xml_node {
public:
	_xml_node() = default;

	virtual ~_xml_node() {}

	virtual string_view node_name() const {
		return "#unknown";
	}

	virtual const _xml_attribute_map *attributes() const {
		return nullptr;
	}

	virtual _xml_attribute_map *attributes() {
		return nullptr;
	}

	virtual const _xml_node_list *child_nodes() const {
		return nullptr;
	}

	virtual _xml_node_list *child_nodes() {
		return nullptr;
	}

	virtual std::string get_inner_text() const {
		return "";
	}

	virtual void set_inner_text(std::string) {}

	virtual std::string get_inner_xml(bool) const {
		return "";
	}

	virtual void set_inner_xml(string_view) {}

	virtual std::string get_outer_xml(bool) const {
		return "";
	}
};

class _xml_container : public _xml_node {
public:
	_xml_container() = default;

	virtual ~_xml_container() {}

	virtual _xml_node_list *child_nodes() {
		return &_cns;
	}

	virtual const _xml_node_list *child_nodes() const {
		return &_cns;
	}

	virtual std::string get_inner_text() const {
		std::string text;
		for (auto &node : _cns) {
			text += node->get_inner_text();
		}
		return text;
	}

	virtual void set_inner_text(std::string) {}

	virtual std::string get_inner_xml(bool xml_style) const {
		std::string xml;
		for (auto &node : _cns) {
			xml += node->get_outer_xml(xml_style);
		}
		return xml;
	}

	virtual void set_inner_xml(string_view xml) {
		_cns = _parse_xml_nodes(xml);
	}

private:
	_xml_node_list _cns;
};

class _xml_element : public _xml_container {
public:
	_xml_element(std::string node_name) : _name(std::move(node_name)) {}

	virtual ~_xml_element() {}

	virtual string_view node_name() const {
		return _name;
	}

	virtual _xml_attribute_map *attributes() {
		return &_attrs;
	}

	virtual const _xml_attribute_map *attributes() const {
		return &_attrs;
	}

	virtual std::string get_outer_xml(bool xml_style) const {
		std::string attrs_str;
		for (auto &attr : _attrs) {
			attrs_str += str_join({" ", attr.first});
			if (attr.second.length() || xml_style) {
				attrs_str += str_join({"=\"", attr.second, "\""});
			}
		}
		if (child_nodes()->empty() && xml_style) {
			return str_join({"<", _name, attrs_str, " />"});
		}
		return str_join(
			{"<",
			 _name,
			 attrs_str,
			 ">",
			 get_inner_xml(xml_style),
			 "</",
			 node_name(),
			 ">"});
	}

private:
	std::string _name;
	std::map<std::string, std::string> _attrs;
};

class _xml_document : public _xml_container {
public:
	_xml_document() = default;

	virtual ~_xml_document() {}

	virtual string_view node_name() const {
		return "#document";
	}

	virtual std::string get_outer_xml(bool xml_style) const {
		return get_inner_xml(xml_style);
	}
};

class _xml_text : public _xml_node {
public:
	_xml_text(std::string content) : _cont(std::move(content)) {}

	virtual ~_xml_text() {}

	virtual string_view node_name() const {
		return "#text";
	}

	virtual std::string get_inner_text() const {
		return _cont;
	}

	virtual void set_inner_text(std::string content) {
		_cont = std::move(content);
	}

	virtual std::string get_inner_xml(bool) const {
		return _cont;
	}

	virtual std::string get_outer_xml(bool) const {
		return _cont;
	}

private:
	std::string _cont;
};

enum class _xml_tag_status : uchar { closed = 0, opening = 1, closing = 2 };

inline _xml_node_list _parse_xml_nodes(string_view xml) {
	_xml_node_list nodes;
	std::list<_xml_node_list::iterator> uncloseds;

	for (size_t i = 0; i < xml.length();) {
		if (xml[i] == '<') {
			++i;

			if (!skip_space(xml, i)) {
				return nodes;
			}

			_xml_tag_status status;

			switch (xml[i]) {

			case '/':
				status = _xml_tag_status::closing;
				++i;
				break;

			case '?':
				status = _xml_tag_status::closed;
				++i;
				break;

			case '!':
				status = _xml_tag_status::closed;
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
				status = _xml_tag_status::opening;
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
			} while (!is_space(xml[i]) && xml[i] != '>' && xml[i] != '/' &&
					 xml[i] != '?');

			auto node_name =
				to_string(xml.substr(node_name_start, i - node_name_start));

			if (status < _xml_tag_status::closing) {
				auto node =
					std::make_shared<_xml_element>(std::move(node_name));

				for (;;) {
					if (!skip_space(xml, i)) {
						return nodes;
					}

					if (xml[i] == '/' || xml[i] == '?') {
						status = _xml_tag_status::closed;
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
					} while (!is_space(xml[i]) && xml[i] != '=' &&
							 xml[i] != '>' && xml[i] != '/' && xml[i] != '?');

					auto attr_name = to_string(
						xml.substr(attr_name_start, i - attr_name_start));

					if (!skip_space(xml, i)) {
						return nodes;
					}

					if (xml[i] != '=') {
						node->attributes()->emplace(std::move(attr_name), "");
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

					node->attributes()->emplace(
						std::move(attr_name),
						to_string(
							xml.substr(attr_val_start, i - attr_val_start)));

					++i;
				}

				nodes.emplace_back(std::move(node));

				if (status == _xml_tag_status::opening) {
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
				if (!(*itit)->type_is<_xml_element>() ||
					(*(*itit))->node_name() != node_name) {
					continue;
				}
				auto it = (*itit);
				auto cns = (*it)->child_nodes();
				cns->splice(cns->end(), nodes, ++it, nodes.end());
				break;
			}
			continue;
		}

		auto text_start = i;
		do {
			++i;
		} while (i < xml.length() && xml[i] != '<');

		auto text = xml.substr(text_start, i - text_start);
		if (is_space(text)) {
			continue;
		}

		nodes.emplace_back(std::make_shared<_xml_text>(
			to_string(xml.substr(text_start, i - text_start))));
	}

	return nodes;
}

class xml_node;

using xml_node_list = std::list<xml_node>;

class xml_node {
public:
	operator bool() const {
		return _n;
	}

	string_view node_name() const {
		assert(_n);

		return _n->node_name();
	}

	string_view get_attribute(const std::string &name) const {
		assert(_n);

		auto attrs = _n->attributes();
		if (!attrs) {
			return "";
		}
		auto it = attrs->find(name);
		if (it == attrs->end()) {
			return "";
		}
		return it->second;
	}

	void set_attribute(const std::string &name, std::string val) {
		assert(_n);

		auto attrs = _n->attributes();
		if (!attrs) {
			return;
		}
		(*attrs)[name] = std::move(val);
	}

	bool each(
		const std::function<bool(xml_node)> &cb,
		bool recursion = false,
		bool skip_text = true) const {
		assert(_n);

		auto cns = _n->child_nodes();
		if (!cns) {
			return true;
		}
		for (auto it = cns->begin(); it != cns->end();) {
			if (skip_text && it->type_is<_xml_text>()) {
				++it;
				continue;
			}
			auto cn = *it;
			++it;
			if (!cb(cn)) {
				return false;
			}
			if (recursion && !xml_node(cn).each(cb, recursion, skip_text)) {
				return false;
			}
		}
		return true;
	}

	xml_node_list
	query_all(string_view selector, size_t max_count = nmax<size_t>()) const {
		assert(_n);

		xml_node_list li;

		each(
			[selector, &li, max_count](xml_node n) -> bool {
				if (n.node_name() != selector) {
					return true;
				}
				li.push_back(std::move(n));
				if (li.size() == max_count) {
					return false;
				}
				return true;
			},
			true);

		return li;
	}

	xml_node query(string_view selector) const {
		assert(_n);

		auto li = query_all(selector, 1);
		if (li.empty()) {
			return xml_node();
		}
		return li.front();
	}

	std::string get_inner_text() const {
		assert(_n);

		return _n->get_inner_text();
	}

	void set_inner_text(std::string text) {
		assert(_n);

		_n->set_inner_text(text);
	}

	std::string get_outer_text() const {
		assert(_n);

		return _n->get_inner_text();
	}

	void set_outer_text(std::string text) {
		_n = std::make_shared<_xml_text>(std::move(text));
	}

	std::string get_inner_xml(bool xml_style = true) const {
		assert(_n);

		return _n->get_inner_xml(xml_style);
	}

	void set_inner_xml(string_view xml) {
		assert(_n);

		_n->set_inner_xml(xml);
	}

	std::string get_outer_xml(bool xml_style = true) const {
		assert(_n);

		return _n->get_outer_xml(xml_style);
	}

	void set_outer_xml(string_view xml) {
		auto nodes = _parse_xml_nodes(xml);
		if (nodes.empty()) {
			return;
		}
		_n = std::move(nodes.front());
	}

private:
	_xml_node_i _n;

	xml_node() = default;

	xml_node(_xml_node_i n) : _n(std::move(n)){};

	friend inline xml_node make_xml_unknown();

	friend inline xml_node make_xml_element(std::string node_name);

	friend inline xml_node make_xml_text(std::string content);

	friend inline xml_node make_xml_document();
};

inline xml_node make_xml_unknown() {
	static _xml_document inst;
	return xml_node(&inst);
}

inline xml_node make_xml_element(std::string node_name) {
	return xml_node(std::make_shared<_xml_element>(std::move(node_name)));
}

inline xml_node make_xml_text(std::string content) {
	return xml_node(std::make_shared<_xml_text>(std::move(content)));
}

inline xml_node make_xml_document() {
	return xml_node(std::make_shared<_xml_document>());
}

inline xml_node parse_xml_document(string_view xml) {
	auto doc = make_xml_document();
	doc.set_inner_xml(xml);
	return doc;
}

inline xml_node parse_xml_node(string_view xml) {
	auto n = make_xml_unknown();
	n.set_outer_xml(xml);
	return n;
}

} // namespace rua

#endif
