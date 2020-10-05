#ifndef _RUA_XML_HPP
#define _RUA_XML_HPP

#include "string.hpp"
#include "types/util.hpp"

#include <list>
#include <map>
#include <memory>

namespace rua {

class xml_node;

enum class xml_node_type : uchar {
	element = 1,
	text = 3,
	document = 9,
};

using xml_node_list = std::list<xml_node>;

inline xml_node make_xml_element(std::string name);
inline xml_node make_xml_text(std::string content);
inline xml_node make_xml_document();

inline xml_node parse_xml_document(string_view xml);
inline xml_node parse_xml_node(string_view xml);
inline xml_node_list parse_xml_node_list(string_view xml);

////////////////////////////////////////////////////////////////////////////

class _xml_node;

using _xml_shared_node = std::shared_ptr<_xml_node>;

using _xml_weak_node = std::weak_ptr<_xml_node>;

using _xml_weak_node_list = std::list<_xml_weak_node>;

using _xml_attribute_map = std::map<std::string, std::string>;

class _xml_node {
public:
	_xml_node() = default;

	virtual ~_xml_node() {}

	virtual xml_node_type type() const = 0;

	virtual string_view name() const = 0;

	virtual const _xml_attribute_map *attributes() const {
		return nullptr;
	}

	virtual _xml_attribute_map *attributes() {
		return nullptr;
	}

	virtual const xml_node_list *child_nodes() const {
		return nullptr;
	}

	virtual xml_node_list *child_nodes() {
		return nullptr;
	}

	virtual const _xml_weak_node_list *parent_nodes() const {
		return nullptr;
	}

	virtual _xml_weak_node_list *parent_nodes() {
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

////////////////////////////////////////////////////////////////////////////

class xml_node {
public:
	constexpr xml_node(std::nullptr_t = nullptr) : _n(nullptr) {}

	operator bool() const {
		return _n.get();
	}

	xml_node_type type() const {
		assert(*this);

		return _n->type();
	}

	string_view name() const {
		assert(*this);

		return _n->name();
	}

	string_view get_attribute(const std::string &name) const {
		assert(*this);

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
		assert(*this);

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
		assert(*this);

		auto cns = _n->child_nodes();
		if (!cns) {
			return true;
		}
		for (auto it = cns->begin(); it != cns->end();) {
			if (skip_text && it->type() == xml_node_type::text) {
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
		assert(*this);

		xml_node_list li;

		each(
			[selector, &li, max_count](xml_node n) -> bool {
				if (n.name() != selector) {
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
		assert(*this);

		auto li = query_all(selector, 1);
		if (li.empty()) {
			return xml_node();
		}
		return li.front();
	}

	bool append(xml_node node, bool dedupe = true) {
		assert(*this);

		auto cns = _n->child_nodes();
		if (!cns) {
			return false;
		}
		if (!node._add_parent_node(*this, dedupe)) {
			return false;
		}
		cns->emplace_back(std::move(node));
		return true;
	}

	bool append(xml_node_list nodes, bool dedupe = true) {
		assert(*this);

		auto cns = _n->child_nodes();
		if (!cns) {
			return false;
		}
		if (!dedupe) {
			for (auto &node : nodes) {
				node._add_parent_node(*this, false);
			}
			cns->merge(std::move(nodes));
			return true;
		}
		for (auto it = nodes.begin(); it != nodes.end();) {
			if (!it->_add_parent_node(*this)) {
				++it;
				continue;
			}
			cns->emplace_back(std::move(*it));
			it = nodes.erase(it);
		}
		return nodes.empty();
	}

	bool set_child_nodes(xml_node_list nodes, bool dedupe = true) {
		assert(*this);

		auto cns = _n->child_nodes();
		if (!cns) {
			return false;
		}
		if (!dedupe) {
			for (auto &node : nodes) {
				node._add_parent_node(*this, false);
			}
			*cns = std::move(nodes);
			return true;
		}
		cns->resize(0);
		for (auto it = nodes.begin(); it != nodes.end();) {
			if (!it->_add_parent_node(*this)) {
				++it;
				continue;
			}
			cns->emplace_back(std::move(*it));
			it = nodes.erase(it);
		}
		return nodes.empty();
	}

	xml_node_list get_child_nodes() const {
		assert(*this);

		auto cns = _n->child_nodes();
		if (!cns) {
			return xml_node_list();
		}
		return *cns;
	}

	xml_node_list get_children() const {
		assert(*this);

		xml_node_list li;
		auto cns = _n->child_nodes();
		if (!cns) {
			return li;
		}
		for (auto &cn : *cns) {
			if (cn.type() != xml_node_type::element) {
				continue;
			}
			li.emplace_back(cn);
		}
		return li;
	}

	xml_node_list get_parent_nodes() const {
		assert(*this);

		xml_node_list li;
		auto pns = _n->parent_nodes();
		if (!pns) {
			return li;
		}
		for (auto &pn : *pns) {
			li.emplace_back(xml_node(pn.lock()));
		}
		return li;
	}

	std::string get_inner_text() const {
		assert(*this);

		return _n->get_inner_text();
	}

	void set_inner_text(std::string text) {
		assert(*this);

		_n->set_inner_text(text);
	}

	std::string get_outer_text() const {
		assert(*this);

		return _n->get_inner_text();
	}

	void set_outer_text(std::string text) {
		*this = make_xml_text(std::move(text));
	}

	std::string get_inner_xml(bool xml_style = true) const {
		assert(*this);

		return _n->get_inner_xml(xml_style);
	}

	void set_inner_xml(string_view xml) {
		assert(*this);

		_n->set_inner_xml(xml);
	}

	std::string get_outer_xml(bool xml_style = true) const {
		assert(*this);

		return _n->get_outer_xml(xml_style);
	}

	void set_outer_xml(string_view xml) {
		auto nodes = parse_xml_node_list(xml);
		if (nodes.empty()) {
			return;
		}
		*this = std::move(nodes.front());
	}

private:
	_xml_shared_node _n;

	xml_node(_xml_shared_node n) : _n(std::move(n)){};

	bool _check_parent_node(const _xml_shared_node &n) const {
		if (!n || n == _n) {
			return false;
		}
		auto pns = n->parent_nodes();
		if (pns) {
			for (auto &pn : *pns) {
				if (!_check_parent_node(pn.lock())) {
					return false;
				}
			}
		}
		auto cns = _n->child_nodes();
		if (!cns) {
			return true;
		}
		for (auto &cn : *cns) {
			if (!cn._check_parent_node(n)) {
				return false;
			}
		}
		return true;
	}

	bool _add_parent_node(xml_node node, bool dedupe = true) {
		auto pns = _n->parent_nodes();
		if (!pns) {
			return false;
		}
		if (!dedupe) {
			pns->emplace_back(std::move(node._n));
			return true;
		}
		for (auto &pn : *pns) {
			if (pn.lock() == node._n) {
				return true;
			}
		}
		if (!_check_parent_node(node._n)) {
			return false;
		}
		pns->emplace_back(std::move(node._n));
		return true;
	}

	friend inline xml_node make_xml_element(std::string name);
	friend inline xml_node make_xml_text(std::string content);
	friend inline xml_node make_xml_document();
};

////////////////////////////////////////////////////////////////////////////

template <typename XmlBaseNode>
class _xml_container : public XmlBaseNode {
public:
	_xml_container() = default;

	virtual ~_xml_container() {}

	virtual xml_node_list *child_nodes() {
		return &_cns;
	}

	virtual const xml_node_list *child_nodes() const {
		return &_cns;
	}

	virtual std::string get_inner_text() const {
		std::string text;
		for (auto &cn : _cns) {
			text += cn.get_inner_text();
		}
		return text;
	}

	virtual void set_inner_text(std::string) {}

	virtual std::string get_inner_xml(bool xml_style) const {
		std::string xml;
		for (auto &cn : _cns) {
			xml += cn.get_outer_xml(xml_style);
		}
		return xml;
	}

	virtual void set_inner_xml(string_view xml) {
		_cns = parse_xml_node_list(xml);
	}

private:
	xml_node_list _cns;
};

class _xml_document : public _xml_container<_xml_node> {
public:
	_xml_document() = default;

	virtual ~_xml_document() {}

	virtual xml_node_type type() const {
		return xml_node_type::document;
	}

	virtual string_view name() const {
		return "#document";
	}

	virtual std::string get_outer_xml(bool xml_style) const {
		return get_inner_xml(xml_style);
	}
};

class _xml_child : public _xml_node {
public:
	_xml_child() = default;

	virtual ~_xml_child() {}

	virtual const _xml_weak_node_list *parent_nodes() const {
		return &_pns;
	}

	virtual _xml_weak_node_list *parent_nodes() {
		return &_pns;
	}

private:
	_xml_weak_node_list _pns;
};

class _xml_element : public _xml_container<_xml_child> {
public:
	_xml_element(std::string name) : _name(std::move(name)) {}

	virtual ~_xml_element() {}

	virtual xml_node_type type() const {
		return xml_node_type::element;
	}

	virtual string_view name() const {
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
			 name(),
			 ">"});
	}

private:
	std::string _name;
	std::map<std::string, std::string> _attrs;
};

class _xml_text : public _xml_child {
public:
	_xml_text(std::string content) : _cont(std::move(content)) {}

	virtual ~_xml_text() {}

	virtual xml_node_type type() const {
		return xml_node_type::text;
	}

	virtual string_view name() const {
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

////////////////////////////////////////////////////////////////////////////

inline xml_node make_xml_element(std::string name) {
	return std::static_pointer_cast<_xml_node>(
		std::make_shared<_xml_element>(std::move(name)));
}

inline xml_node make_xml_text(std::string content) {
	return std::static_pointer_cast<_xml_node>(
		std::make_shared<_xml_text>(std::move(content)));
}

inline xml_node make_xml_document() {
	return std::static_pointer_cast<_xml_node>(
		std::make_shared<_xml_document>());
}

inline xml_node parse_xml_document(string_view xml) {
	auto doc = make_xml_document();
	doc.set_inner_xml(xml);
	return doc;
}

inline xml_node parse_xml_node(string_view xml) {
	xml_node n;
	n.set_outer_xml(xml);
	return n;
}

inline xml_node_list parse_xml_node_list(string_view xml) {
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
			} while (!is_space(xml[i]) && xml[i] != '>' && xml[i] != '/' &&
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
					} while (!is_space(xml[i]) && xml[i] != '=' &&
							 xml[i] != '>' && xml[i] != '/' && xml[i] != '?');

					auto attr_name = to_string(
						xml.substr(attr_name_start, i - attr_name_start));

					if (!skip_space(xml, i)) {
						return nodes;
					}

					if (xml[i] != '=') {
						node.set_attribute(attr_name, "");
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

					node.set_attribute(
						attr_name,
						to_string(
							xml.substr(attr_val_start, i - attr_val_start)));

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
				if (n.type() != xml_node_type::element || n.name() != name) {
					continue;
				}
				xml_node_list cns;
				cns.splice(cns.end(), nodes, ++it, nodes.end());
				n.set_child_nodes(std::move(cns), false);
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
		if (is_space(text)) {
			continue;
		}

		nodes.emplace_back(
			make_xml_text(to_string(xml.substr(text_start, i - text_start))));
	}

	return nodes;
}

} // namespace rua

#endif
