#ifndef _RUA_STRING_CHAR_SET_HPP
#define _RUA_STRING_CHAR_SET_HPP

#include "view.hpp"

#include "../macros.hpp"
#include "../types/util.hpp"

namespace rua {

using rune_t = uint32_t;

namespace rune {

/*
	Reference from
		https://en.wiktionary.org/wiki/Appendix:Control_characters
*/

//////////////////// C0 ////////////////////

// control

RUA_INLINE_CONST rune_t nul = 0;
RUA_INLINE_CONST rune_t soh = 1;
RUA_INLINE_CONST rune_t stx = 2;
RUA_INLINE_CONST rune_t etx = 3;
RUA_INLINE_CONST rune_t eot = 4;
RUA_INLINE_CONST rune_t enq = 5;
RUA_INLINE_CONST rune_t ack = 6;
RUA_INLINE_CONST rune_t bel = 7;
RUA_INLINE_CONST rune_t bs = 8;
RUA_INLINE_CONST rune_t ht = 9;
RUA_INLINE_CONST rune_t lf = 10;
RUA_INLINE_CONST rune_t vt = 11;
RUA_INLINE_CONST rune_t ff = 12;
RUA_INLINE_CONST rune_t cr = 13;
RUA_INLINE_CONST rune_t so = 14;
RUA_INLINE_CONST rune_t si = 15;
RUA_INLINE_CONST rune_t dle = 16;
RUA_INLINE_CONST rune_t dc1 = 17;
RUA_INLINE_CONST rune_t dc2 = 18;
RUA_INLINE_CONST rune_t dc3 = 19;
RUA_INLINE_CONST rune_t dc4 = 20;
RUA_INLINE_CONST rune_t nak = 21;
RUA_INLINE_CONST rune_t syn = 22;
RUA_INLINE_CONST rune_t etb = 23;
RUA_INLINE_CONST rune_t can = 24;
RUA_INLINE_CONST rune_t em = 25;
RUA_INLINE_CONST rune_t sub = 26;
RUA_INLINE_CONST rune_t esc = 27;
RUA_INLINE_CONST rune_t fs = 28;
RUA_INLINE_CONST rune_t gs = 29;
RUA_INLINE_CONST rune_t rs = 30;
RUA_INLINE_CONST rune_t us = 31;

RUA_INLINE_CONST rune_t del = 127;

// space

RUA_INLINE_CONST rune_t sp = 32;

//////////////////// C1 ////////////////////

// control

RUA_INLINE_CONST rune_t pad = 128;
RUA_INLINE_CONST rune_t hop = 129;
RUA_INLINE_CONST rune_t bph = 130;
RUA_INLINE_CONST rune_t nbh = 131;
RUA_INLINE_CONST rune_t ind = 132;
RUA_INLINE_CONST rune_t nel = 133;
RUA_INLINE_CONST rune_t ssa = 134;
RUA_INLINE_CONST rune_t esa = 135;
RUA_INLINE_CONST rune_t hts = 136;
RUA_INLINE_CONST rune_t htj = 137;
RUA_INLINE_CONST rune_t vts = 138;
RUA_INLINE_CONST rune_t pld = 139;
RUA_INLINE_CONST rune_t plu = 140;
RUA_INLINE_CONST rune_t ri = 141;
RUA_INLINE_CONST rune_t ss2 = 142;
RUA_INLINE_CONST rune_t ss3 = 143;
RUA_INLINE_CONST rune_t dcs = 144;
RUA_INLINE_CONST rune_t pu1 = 145;
RUA_INLINE_CONST rune_t pu2 = 146;
RUA_INLINE_CONST rune_t sts = 147;
RUA_INLINE_CONST rune_t cch = 148;
RUA_INLINE_CONST rune_t mw = 149;
RUA_INLINE_CONST rune_t spa = 150;
RUA_INLINE_CONST rune_t epa = 151;
RUA_INLINE_CONST rune_t sos = 152;
RUA_INLINE_CONST rune_t sgci = 153;
RUA_INLINE_CONST rune_t sci = 154;
RUA_INLINE_CONST rune_t csi = 155;
RUA_INLINE_CONST rune_t st = 156;
RUA_INLINE_CONST rune_t osc = 157;
RUA_INLINE_CONST rune_t pm = 158;
RUA_INLINE_CONST rune_t apc = 159;

///////////////// ISO-8859 /////////////////

// control

RUA_INLINE_CONST rune_t shy = 173;

// space

RUA_INLINE_CONST rune_t nbsp = 160;

////////////////// UNICODE //////////////////

// control

RUA_INLINE_CONST rune_t cgj = 847;

RUA_INLINE_CONST rune_t sam = 1807;

RUA_INLINE_CONST rune_t mvs = 6158;

RUA_INLINE_CONST rune_t zwsp = 8203;
RUA_INLINE_CONST rune_t zwnj = 8204;
RUA_INLINE_CONST rune_t zwj = 8205;
RUA_INLINE_CONST rune_t lrm = 8206;
RUA_INLINE_CONST rune_t rlm = 8207;
RUA_INLINE_CONST rune_t ls = 8232;
RUA_INLINE_CONST rune_t ps = 8233;
RUA_INLINE_CONST rune_t lre = 8234;
RUA_INLINE_CONST rune_t rle = 8235;
RUA_INLINE_CONST rune_t pdf = 8236;
RUA_INLINE_CONST rune_t lro = 8237;
RUA_INLINE_CONST rune_t rlo = 8238;

RUA_INLINE_CONST rune_t wj = 8288;
RUA_INLINE_CONST rune_t fa = 8289;
RUA_INLINE_CONST rune_t it = 8290;
RUA_INLINE_CONST rune_t is = 8291;
RUA_INLINE_CONST rune_t ip = 8292;

RUA_INLINE_CONST rune_t iss = 8298;
RUA_INLINE_CONST rune_t ass = 8299;
RUA_INLINE_CONST rune_t iafs = 8300;
RUA_INLINE_CONST rune_t aafs = 8301;
RUA_INLINE_CONST rune_t nads = 8302;
RUA_INLINE_CONST rune_t nods = 8303;

RUA_INLINE_CONST rune_t zwnbsp = 65279;

RUA_INLINE_CONST rune_t iaa = 65529;
RUA_INLINE_CONST rune_t ias = 65530;
RUA_INLINE_CONST rune_t iat = 65531;

// space

RUA_INLINE_CONST rune_t ospmk = 5760;

RUA_INLINE_CONST rune_t w1en = 8192;
RUA_INLINE_CONST rune_t w1em = 8193;
RUA_INLINE_CONST rune_t w1ensp = 8194;
RUA_INLINE_CONST rune_t w1emsp = 8195;
RUA_INLINE_CONST rune_t w3emsp = 8196;
RUA_INLINE_CONST rune_t w4emsp = 8197;
RUA_INLINE_CONST rune_t w6emsp = 8198;
RUA_INLINE_CONST rune_t w1mdsp = 8199;
RUA_INLINE_CONST rune_t w1npsp = 8200;
RUA_INLINE_CONST rune_t w1o5emsp = 8201;
RUA_INLINE_CONST rune_t w1o10emsp = 8202;

RUA_INLINE_CONST rune_t nnbsp = 8239;

RUA_INLINE_CONST rune_t mmsp = 8287;

RUA_INLINE_CONST rune_t w1cjksp = 12288;

} // namespace rune

inline constexpr bool is_control(const rune_t c) {
	return c < rune::sp || c == rune::del ||
		   (c >= rune::nel && c <= rune::apc) || c == rune::shy ||
		   c == rune::cgj || c == rune::sam || c == rune::mvs ||
		   (c >= rune::zwsp && c <= rune::rlo) ||
		   (c >= rune::wj && c <= rune::ip) ||
		   (c >= rune::iss && c <= rune::nods) || c == rune::zwnbsp ||
		   (c >= rune::iaa && c <= rune::iat);
}

inline RUA_CONSTEXPR_14 bool is_control(string_view sv) {
	if (sv.empty()) {
		return false;
	}
	for (auto &c : sv) {
		if (!is_control(c)) {
			return false;
		}
	}
	return true;
}

inline constexpr bool is_space(const rune_t c) {
	return c == rune::sp || is_control(c) || c == rune::nbsp ||
		   c == rune::ospmk || (c >= rune::w1en && c <= rune::w1o10emsp) ||
		   c == rune::nnbsp || c == rune::mmsp || c == rune::w1cjksp;
}

inline RUA_CONSTEXPR_14 bool is_space(string_view sv) {
	for (auto &c : sv) {
		if (!is_space(c)) {
			return false;
		}
	}
	return true;
}

namespace eol {

RUA_INLINE_CONST const char *lf = "\n";
RUA_INLINE_CONST const char *crlf = "\r\n";
RUA_INLINE_CONST const char *cr = "\r";

#ifdef _WIN32

RUA_INLINE_CONST const char *sys_text = crlf;
RUA_INLINE_CONST const char *sys_con = crlf;

#elif defined(RUA_DARWIN)

RUA_INLINE_CONST const char *sys_text = cr;
RUA_INLINE_CONST const char *sys_con = lf;

#else

RUA_INLINE_CONST const char *sys_text = lf;
RUA_INLINE_CONST const char *sys_con = lf;

#endif

} // namespace eol

inline constexpr bool is_eol(const rune_t c) {
	return c == rune::lf || c == rune::cr;
}

inline RUA_CONSTEXPR_14 bool is_eol(string_view sv) {
	if (sv.empty()) {
		return false;
	}
	for (auto &c : sv) {
		if (!is_eol(c)) {
			return false;
		}
	}
	return true;
}

} // namespace rua

#endif
