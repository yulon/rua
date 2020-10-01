#ifndef _RUA_XML_HPP
#define _RUA_XML_HPP

#include "interface_ptr.hpp"
#include "string.hpp"

#include <list>
#include <map>

namespace rua {

class _xml_node;

using _xml_node_i = interface_ptr<_xml_node>;

using _xml_attribute_map = std::map<std::string, std::string>;

using _xml_node_list = std::list<_xml_node_i>;

inline _xml_node_list _xml_parse(string_view xml);

class _xml_node {
public:
	_xml_node() = default;

	virtual ~_xml_node() {}

	virtual string_view name() const = 0;

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

class _xml_unclosed_tag_node : public _xml_node {
public:
	_xml_unclosed_tag_node(std::string name) : _name(std::move(name)) {}

	virtual ~_xml_unclosed_tag_node() {}

	virtual string_view name() const {
		return _name;
	}

	virtual _xml_attribute_map *attributes() {
		return &_attributes;
	}

	virtual const _xml_attribute_map *attributes() const {
		return &_attributes;
	}

	virtual std::string get_outer_xml(bool slash_in_unclosed_tag) const {
		if (_attributes.size()) {
			std::string r = str_join({"<", _name});
			for (auto &attr : _attributes) {
				r += str_join({" ", attr.first});
				if (attr.second.length()) {
					r += str_join({"=\"", attr.second, "\""});
				}
			}
			if (slash_in_unclosed_tag) {
				r += '/';
			}
			r += '>';
			return r;
		}
		if (slash_in_unclosed_tag) {
			return str_join({"<", _name, "/>"});
		}
		return str_join({"<", _name, ">"});
	}

private:
	std::string _name;
	std::map<std::string, std::string> _attributes;
};

class _xml_tag_node : public _xml_unclosed_tag_node {
public:
	_xml_tag_node(std::string name) : _xml_unclosed_tag_node(std::move(name)) {}

	virtual ~_xml_tag_node() {}

	virtual _xml_node_list *child_nodes() {
		return &_child_nodes;
	}

	virtual const _xml_node_list *child_nodes() const {
		return &_child_nodes;
	}

	virtual std::string get_inner_text() const {
		std::string text;
		for (auto &node : _child_nodes) {
			text += node->get_inner_text();
		}
		return text;
	}

	virtual void set_inner_text(std::string) {}

	virtual std::string get_inner_xml(bool slash_in_unclosed_tag) const {
		std::string xml;
		for (auto &node : _child_nodes) {
			xml += node->get_outer_xml(slash_in_unclosed_tag);
		}
		return xml;
	}

	virtual void set_inner_xml(string_view xml) {
		_child_nodes = _xml_parse(xml);
	}

	virtual std::string get_outer_xml(bool slash_in_unclosed_tag) const {
		return str_join(
			{_xml_unclosed_tag_node::get_outer_xml(false),
			 get_inner_xml(slash_in_unclosed_tag),
			 "</",
			 name(),
			 ">"});
	}

private:
	_xml_node_list _child_nodes;
};

class _xml_document_node : public _xml_node {
public:
	_xml_document_node() = default;

	virtual ~_xml_document_node() {}

	virtual string_view name() const {
		return "#document";
	}

	virtual _xml_node_list *child_nodes() {
		return &_child_nodes;
	}

	virtual const _xml_node_list *child_nodes() const {
		return &_child_nodes;
	}

	virtual std::string get_inner_text() const {
		std::string text;
		for (auto &node : _child_nodes) {
			text += node->get_inner_text();
		}
		return text;
	}

	virtual void set_inner_text(std::string) {}

	virtual std::string get_inner_xml(bool slash_in_unclosed_tag) const {
		std::string xml;
		for (auto &node : _child_nodes) {
			xml += node->get_outer_xml(slash_in_unclosed_tag);
		}
		return xml;
	}

	virtual void set_inner_xml(string_view xml) {
		_child_nodes = _xml_parse(xml);
	}

	virtual std::string get_outer_xml(bool slash_in_unclosed_tag) const {
		return get_inner_xml(slash_in_unclosed_tag);
	}

private:
	_xml_node_list _child_nodes;
};

class _xml_text_node : public _xml_node {
public:
	_xml_text_node(std::string content) : _content(std::move(content)) {}

	virtual ~_xml_text_node() {}

	virtual string_view name() const {
		return "#text";
	}

	virtual std::string get_inner_text() const {
		return _content;
	}

	virtual void set_inner_text(std::string content) {
		_content = std::move(content);
	}

	virtual std::string get_inner_xml(bool) const {
		return _content;
	}

	virtual std::string get_outer_xml(bool) const {
		return _content;
	}

private:
	std::string _content;
};

bool _xml_skip_space(string_view sv, size_t &i) {
	while (is_space(sv[i])) {
		++i;
		if (i >= sv.length()) {
			return false;
		}
	};
	return true;
}

inline _xml_node_list _xml_parse(string_view xml) {
	_xml_node_list nodes;

	for (size_t i = 0; i < xml.length();) {
		if (xml[i] == '<') {
			++i;

			if (!_xml_skip_space(xml, i)) {
				return nodes;
			}

			bool is_positive = true;
			switch (xml[i]) {
			case '/':
				is_positive = false;
				RUA_FALLTHROUGH;
			case '?':
				++i;
				break;
			case '!':
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
			}

			if (!_xml_skip_space(xml, i)) {
				return nodes;
			}

			auto name_start = i;
			do {
				++i;
				if (i >= xml.length()) {
					return nodes;
				}
			} while (!is_space(xml[i]) && xml[i] != '>');

			auto name = to_string(xml.substr(name_start, i - name_start));

			if (is_positive) {
				auto node =
					std::make_shared<_xml_unclosed_tag_node>(std::move(name));

				for (;;) {
					if (!_xml_skip_space(xml, i)) {
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
							 xml[i] != '>');

					auto attr_name = to_string(
						xml.substr(attr_name_start, i - attr_name_start));

					if (!_xml_skip_space(xml, i)) {
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
				continue;
			}

			while (xml[i] != '>') {
				++i;
				if (i >= xml.length()) {
					return nodes;
				}
			}
			++i;

			// bool found = false;
			for (auto rit = --nodes.end(); rit != --nodes.begin(); --rit) {
				if (!rit->type_is<_xml_unclosed_tag_node>() ||
					rit->as<_xml_unclosed_tag_node>()->name() != name) {
					continue;
				}
				auto closed_tag = std::make_shared<_xml_tag_node>(name);
				*closed_tag->attributes() = std::move(*((*rit)->attributes()));
				(*rit) = closed_tag;
				while (--nodes.end() != rit) {
					closed_tag->child_nodes()->emplace_front(
						std::move(nodes.back()));
					nodes.pop_back();
				}
				// found = true;
				break;
			}
			// assert(found);
			continue;
		}

		auto text_start = i;
		do {
			++i;
		} while (i < xml.length() && xml[i] != '<');

		nodes.emplace_back(std::make_shared<_xml_text_node>(
			to_string(xml.substr(text_start, i - text_start))));
	}

	return nodes;
}

class xml_node {
public:
	operator bool() const {
		return _n;
	}

	string_view name() const {
		assert(_n);

		return _n->name();
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
			if (skip_text && it->type_is<_xml_text_node>()) {
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

	std::list<xml_node>
	query_all(string_view selector, size_t max_count = nmax<size_t>()) const {
		assert(_n);

		std::list<xml_node> li;

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
		_n = std::make_shared<_xml_text_node>(std::move(text));
	}

	std::string get_inner_xml(bool slash_in_unclosed_tag = true) const {
		assert(_n);

		return _n->get_inner_xml(slash_in_unclosed_tag);
	}

	void set_inner_xml(string_view xml) {
		assert(_n);

		_n->set_inner_xml(xml);
	}

	std::string get_outer_xml(bool slash_in_unclosed_tag = true) const {
		assert(_n);

		return _n->get_outer_xml(slash_in_unclosed_tag);
	}

	void set_outer_xml(string_view xml) {
		auto nodes = _xml_parse(xml);
		if (nodes.empty()) {
			return;
		}
		_n = std::move(nodes.front());
	}

private:
	_xml_node_i _n;

	xml_node() = default;

	xml_node(_xml_node_i n) : _n(std::move(n)){};

	friend inline xml_node xml_parse(string_view);
};

inline xml_node xml_parse(string_view xml) {
	xml_node doc(std::make_shared<_xml_document_node>());
	doc.set_inner_xml(xml);
	return doc;
}

} // namespace rua

#endif
