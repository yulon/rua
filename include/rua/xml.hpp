#ifndef _RUA_XML_HPP
#define _RUA_XML_HPP

#include "range.hpp"
#include "string.hpp"
#include "types/util.hpp"

#include <list>
#include <map>
#include <memory>

namespace rua {

class xml_node;

using xml_node_list = std::list<xml_node>;

enum class xml_type : uchar {
	element = 1,
	text = 3,
	document = 9,
};

inline xml_node make_xml_element(std::string name);
inline xml_node make_xml_text(std::string content);
inline xml_node make_xml_document();

inline xml_node parse_xml_document(string_view xml);
inline xml_node parse_xml_node(string_view xml);
inline xml_node_list parse_xml_nodes(string_view xml);

////////////////////////////////////////////////////////////////////////////

class _xml_node {
public:
	_xml_node() = default;

	virtual ~_xml_node() {}

	virtual xml_type type() const = 0;

	virtual string_view name() const = 0;

	virtual const std::map<std::string, std::string> *attributes() const {
		return nullptr;
	}

	virtual std::map<std::string, std::string> *attributes() {
		return nullptr;
	}

	virtual const xml_node_list *child_nodes() const {
		return nullptr;
	}

	virtual xml_node_list *child_nodes() {
		return nullptr;
	}

	virtual const std::weak_ptr<_xml_node> *parent() const {
		return nullptr;
	}

	virtual std::weak_ptr<_xml_node> *parent() {
		return nullptr;
	}

	virtual const xml_node_list::iterator *iterator() const {
		return nullptr;
	}

	virtual xml_node_list::iterator *iterator() {
		return nullptr;
	}

	virtual std::string get_inner_text() const {
		return "";
	}

	virtual std::string get_inner_xml(bool) const {
		return "";
	}

	virtual std::string get_outer_xml(bool) const {
		return "";
	}
};

////////////////////////////////////////////////////////////////////////////

class xml_node_collection {
public:
	using iterator = xml_node_list::iterator;

	xml_node_collection() = default;

	iterator begin() const {
		if (!_n_sp) {
			return iterator();
		}
		auto cns_p = _n_sp->child_nodes();
		if (!cns_p) {
			return iterator();
		}
		return cns_p->begin();
	}

	iterator end() const {
		if (!_n_sp) {
			return iterator();
		}
		auto cns_p = _n_sp->child_nodes();
		if (!cns_p) {
			return iterator();
		}
		return cns_p->end();
	}

	size_t size() const {
		if (!_n_sp) {
			return 0;
		}
		auto cns_p = _n_sp->child_nodes();
		if (!cns_p) {
			return 0;
		}
		return cns_p->size();
	}

private:
	std::shared_ptr<_xml_node> _n_sp;

	xml_node_collection(std::shared_ptr<_xml_node> n_sp) :
		_n_sp(std::move(n_sp)) {}

	friend xml_node;
};

class xml_element_collection {
public:
	class iterator : public xml_node_list::iterator {
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
		xml_node_list::iterator _end;

		iterator(xml_node_list::iterator it, xml_node_list::iterator end);

		friend xml_element_collection;
	};

	xml_element_collection() = default;

	iterator begin() const {
		if (!_n_sp) {
			return iterator();
		}
		auto cns_p = _n_sp->child_nodes();
		if (!cns_p) {
			return iterator();
		}
		return iterator(cns_p->begin(), cns_p->end());
	}

	iterator end() const {
		if (!_n_sp) {
			return iterator();
		}
		auto cns_p = _n_sp->child_nodes();
		if (!cns_p) {
			return iterator();
		}
		return iterator(cns_p->end(), cns_p->end());
	}

	size_t size() const {
		return std::distance(begin(), end());
	}

private:
	std::shared_ptr<_xml_node> _n_sp;

	xml_element_collection(std::shared_ptr<_xml_node> n_sp) :
		_n_sp(std::move(n_sp)) {}

	friend xml_node;
};

class xml_node {
public:
	constexpr xml_node(std::nullptr_t = nullptr) : _sp(nullptr) {}

	operator bool() const {
		return _sp.get();
	}

	xml_type type() const {
		assert(*this);

		return _sp->type();
	}

	string_view name() const {
		assert(*this);

		return _sp->name();
	}

	string_view get_attribute(const std::string &name) const {
		assert(*this);

		auto attrs_p = _sp->attributes();
		if (!attrs_p) {
			return "";
		}
		auto it = attrs_p->find(name);
		if (it == attrs_p->end()) {
			return "";
		}
		return it->second;
	}

	void set_attribute(const std::string &name, std::string val) {
		assert(*this);

		auto attrs_p = _sp->attributes();
		if (!attrs_p) {
			return;
		}
		(*attrs_p)[name] = std::move(val);
	}

	xml_node_collection get_child_nodes() const {
		assert(*this);

		return _sp;
	}

	template <
		typename NodeList,
		typename = enable_if_t<std::is_same<
			typename range_traits<NodeList &&>::value_type,
			xml_node>::value>>
	size_t set_child_nodes(NodeList &&nodes, bool check_circular_ref = true) {
		auto cns_p = _sp->child_nodes();
		if (!cns_p) {
			return false;
		}
		cns_p->resize(0);
		return _append_child(
			cns_p, std::forward<NodeList>(nodes), check_circular_ref);
	}

	xml_element_collection get_children() const {
		assert(*this);

		return _sp;
	}

	xml_node_list
	querys(string_view selector, size_t max_count = nmax<size_t>()) const {
		assert(*this);

		xml_node_list matcheds;

		if (!max_count) {
			return matcheds;
		}

		auto cns = get_children();
		if (!cns.size()) {
			return matcheds;
		}

		for (auto it = cns.begin(); it != cns.end(); ++it) {
			if (it->name() == selector) {
				matcheds.emplace_back(*it);
			}

			auto child_matcheds = it->querys(
				selector,
				max_count == nmax<size_t>() ? max_count
											: max_count - matcheds.size());
			if (child_matcheds.size()) {
				matcheds.splice(matcheds.end(), std::move(child_matcheds));
			}

			if (matcheds.size() == max_count) {
				return matcheds;
			}
		}

		return matcheds;
	}

	xml_node query(string_view selector) const {
		assert(*this);

		auto li = querys(selector, 1);
		if (li.empty()) {
			return nullptr;
		}
		return li.front();
	}

	bool append_child(xml_node node, bool check_circular_ref = true) {
		assert(*this);

		auto cns_p = _sp->child_nodes();
		if (!cns_p) {
			return false;
		}
		if (!node ||
			!_append_child(cns_p, std::move(node), check_circular_ref)) {
			return false;
		}
		return true;
	}

	template <
		typename NodeList,
		typename = enable_if_t<std::is_same<
			typename range_traits<NodeList &&>::value_type,
			xml_node>::value>>
	size_t append_child(NodeList &&nodes, bool check_circular_ref = true) {
		assert(*this);

		auto cns_p = _sp->child_nodes();
		if (!cns_p) {
			return false;
		}
		return _append_child(
			cns_p, std::forward<NodeList>(nodes), check_circular_ref);
	}

	bool remove_child(xml_node child) {
		assert(*this);

		auto cn_pn_wp_p = child._sp->parent();
		if (!cn_pn_wp_p) {
			return false;
		}
		if (!cn_pn_wp_p->use_count() || cn_pn_wp_p->lock() != _sp) {
			return false;
		}

		if (!child._detach_parent(_sp)) {
			return false;
		}
		cn_pn_wp_p->reset();
		return true;
	}

	bool replace_child(xml_node new_child, xml_node old_child) {
		assert(*this);

		auto old_cn_pn_wp_p = old_child._sp->parent();
		if (!old_cn_pn_wp_p) {
			return false;
		}
		if (!old_cn_pn_wp_p->use_count() || old_cn_pn_wp_p->lock() != _sp) {
			return false;
		}

		auto cns_p = _sp->child_nodes();
		assert(cns_p);
		auto it = old_child._iterator(cns_p);

		if (!old_child._set_parent(nullptr)) {
			return false;
		}

		if (!new_child._detach_and_set_parent(_sp)) {
			old_child._set_parent(_sp);
			return false;
		}

		*it = std::move(new_child);

		auto it_p = new_child._sp->iterator();
		if (it_p) {
			*it_p = it;
		}
		return true;
	}

	xml_node parent() const {
		assert(*this);

		auto pn_wp_p = _sp->parent();
		if (!pn_wp_p || !pn_wp_p->use_count()) {
			return nullptr;
		}
		return pn_wp_p->lock();
	}

	bool detach() {
		assert(*this);

		return _detach_and_set_parent(nullptr);
	}

	std::string get_inner_text() const {
		assert(*this);

		return _sp->get_inner_text();
	}

	void set_inner_text(std::string) {}

	std::string get_outer_text() const {
		assert(*this);

		return _sp->get_inner_text();
	}

	void set_outer_text(std::string text) {
		*this = make_xml_text(std::move(text));
	}

	std::string get_inner_xml(bool xml_style = true) const {
		assert(*this);

		return _sp->get_inner_xml(xml_style);
	}

	void set_inner_xml(string_view xml) {
		set_child_nodes(parse_xml_nodes(xml), false);
	}

	std::string get_outer_xml(bool xml_style = true) const {
		assert(*this);

		return _sp->get_outer_xml(xml_style);
	}

	void set_outer_xml(string_view xml) {
		auto nodes = parse_xml_nodes(xml);
		if (nodes.empty()) {
			return;
		}
		*this = std::move(nodes.front());
	}

	bool operator==(const xml_node &n) const {
		return _sp == n._sp;
	}

	bool operator!=(const xml_node &n) const {
		return _sp != n._sp;
	}

private:
	std::shared_ptr<_xml_node> _sp;

	xml_node(std::shared_ptr<_xml_node> n) : _sp(std::move(n)){};

	xml_node_list::iterator _iterator(xml_node_list *pn_cns_p) const {
		xml_node_list::iterator it;
		auto it_p = _sp->iterator();
		if (it_p) {
			it = *it_p;
			assert(it != pn_cns_p->end());
			assert(it->_sp == _sp);
			return it;
		}
		for (it = pn_cns_p->begin(); it != pn_cns_p->end(); ++it) {
			if (it->_sp == _sp) {
				break;
			}
		}
		return it;
	}

	bool _detach_parent(const std::shared_ptr<_xml_node> &pn_sp) {
		auto pncns_p = pn_sp->child_nodes();
		assert(pncns_p);

		auto it = _iterator(pncns_p);
		if (it == pncns_p->end()) {
			return false;
		}

		pncns_p->erase(it);
		return true;
	}

	bool
	_check_parent_candidate(const std::shared_ptr<_xml_node> &new_pn_sp) const {
		assert(new_pn_sp);

		if (new_pn_sp == _sp) {
			return false;
		}
		auto new_pn_pn_wp_p = new_pn_sp->parent();
		if (new_pn_pn_wp_p && new_pn_pn_wp_p->use_count()) {
			if (!_check_parent_candidate(new_pn_pn_wp_p->lock())) {
				return false;
			}
		}
		auto cns_p = _sp->child_nodes();
		if (!cns_p) {
			return true;
		}
		for (auto &cn : *cns_p) {
			if (!cn._check_parent_candidate(new_pn_sp)) {
				return false;
			}
		}
		return true;
	}

	bool _set_parent(
		const std::shared_ptr<_xml_node> &new_pn_sp,
		bool check_circular_ref = true) {

		auto pn_wp_p = _sp->parent();
		if (!pn_wp_p) {
			return false;
		}
		if (!new_pn_sp) {
			pn_wp_p->reset();
			return true;
		}
		if (check_circular_ref && !_check_parent_candidate(new_pn_sp)) {
			return false;
		}
		*pn_wp_p = new_pn_sp;
		return true;
	}

	bool _detach_and_set_parent(
		const std::shared_ptr<_xml_node> &new_pn_sp,
		bool check_circular_ref = true) {

		auto pn_wp_p = _sp->parent();
		if (!pn_wp_p) {
			return false;
		}

		if (!pn_wp_p->use_count()) {
			if (!new_pn_sp) {
				pn_wp_p->reset();
				return true;
			}
			if (check_circular_ref && !_check_parent_candidate(new_pn_sp)) {
				return false;
			}
			*pn_wp_p = new_pn_sp;
			return true;
		}

		auto old_pn_sp = pn_wp_p->lock();
		if (old_pn_sp == new_pn_sp) {
			return true;
		}

		if (!new_pn_sp) {
			if (old_pn_sp && !_detach_parent(old_pn_sp)) {
				return false;
			}
			pn_wp_p->reset();
			return true;
		}

		if (check_circular_ref && !_check_parent_candidate(new_pn_sp)) {
			return false;
		}
		if (old_pn_sp && !_detach_parent(old_pn_sp)) {
			return false;
		}
		*pn_wp_p = new_pn_sp;
		return true;
	}

	bool _append_child(
		xml_node_list *cns_p, xml_node cn, bool check_circular_ref = true) {
		assert(cns_p);

		if (!cn._detach_and_set_parent(_sp, check_circular_ref)) {
			return false;
		}

		cns_p->emplace_back(std::move(cn));

		auto it_p = _sp->iterator();
		if (it_p) {
			*it_p = --cns_p->end();
		}
		return true;
	}

	template <
		typename NodeList,
		typename = enable_if_t<std::is_same<
			typename range_traits<NodeList &&>::value_type,
			xml_node>::value>>
	size_t _append_child(
		xml_node_list *cns_p, NodeList &&cns, bool check_circular_ref = true) {
		assert(*this);

		auto cns_sz = cns.size();
		if (!cns_sz) {
			return false;
		}

		size_t appended_c = 0;
		for (auto &cn : std::forward<NodeList>(cns)) {
			if (!cn || !_append_child(cns_p, cn, check_circular_ref)) {
				continue;
			}
			++appended_c;
		}
		return appended_c;
	}

	friend inline xml_node make_xml_element(std::string name);
	friend inline xml_node make_xml_text(std::string content);
	friend inline xml_node make_xml_document();
};

inline xml_element_collection::iterator::iterator(
	xml_node_list::iterator it, xml_node_list::iterator end) :
	xml_node_list::iterator(std::move(it)), _end(std::move(end)) {
	while (*this != _end && (*this)->type() != xml_type::element) {
		xml_node_list::iterator::operator++();
	}
}

inline xml_element_collection::iterator &
xml_element_collection::iterator::operator++() {
	do {
		xml_node_list::iterator::operator++();
	} while (*this != _end && (*this)->type() != xml_type::element);
	return *this;
}

inline xml_element_collection::iterator &
xml_element_collection::iterator::operator--() {
	do {
		xml_node_list::iterator::operator--();
		if (*this == _end) {
			break;
		}
	} while (*this != _end && (*this)->type() != xml_type::element);
	return *this;
}

////////////////////////////////////////////////////////////////////////////

template <typename XmlBaseNode>
class _xml_parent : public XmlBaseNode {
public:
	_xml_parent() = default;

	virtual ~_xml_parent() {}

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

	virtual std::string get_inner_xml(bool xml_style) const {
		std::string xml;
		for (auto &cn : _cns) {
			xml += cn.get_outer_xml(xml_style);
		}
		return xml;
	}

private:
	xml_node_list _cns;
};

class _xml_document : public _xml_parent<_xml_node> {
public:
	_xml_document() = default;

	virtual ~_xml_document() {}

	virtual xml_type type() const {
		return xml_type::document;
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

	virtual const std::weak_ptr<_xml_node> *parent() const {
		return &_pnw;
	}

	virtual std::weak_ptr<_xml_node> *parent() {
		return &_pnw;
	}

	virtual const xml_node_list::iterator *iterator() const {
		return &_it;
	}

	virtual xml_node_list::iterator *iterator() {
		return &_it;
	}

private:
	std::weak_ptr<_xml_node> _pnw;
	xml_node_list::iterator _it;
};

class _xml_element : public _xml_parent<_xml_child> {
public:
	_xml_element(std::string name) : _name(std::move(name)) {}

	virtual ~_xml_element() {}

	virtual xml_type type() const {
		return xml_type::element;
	}

	virtual string_view name() const {
		return _name;
	}

	virtual std::map<std::string, std::string> *attributes() {
		return &_attrs;
	}

	virtual const std::map<std::string, std::string> *attributes() const {
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

	virtual xml_type type() const {
		return xml_type::text;
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
				if (n.type() != xml_type::element || n.name() != name) {
					continue;
				}
				xml_node_list cns;
				cns.splice(cns.end(), nodes, ++it, nodes.end());
				n.set_child_nodes(cns, false);
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
