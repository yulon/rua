#ifndef _RUA_STRING_CHAR_SET_HPP
#define _RUA_STRING_CHAR_SET_HPP

#include "view.hpp"

#include "../macros.hpp"
#include "../types/util.hpp"

namespace rua {

namespace char_code {

/*
	Reference from
		https://en.wiktionary.org/wiki/Appendix:Control_characters
*/

//////////////////// C0 ////////////////////

// control

RUA_CVAL char32_t nul = 0;
RUA_CVAL char32_t soh = 1;
RUA_CVAL char32_t stx = 2;
RUA_CVAL char32_t etx = 3;
RUA_CVAL char32_t eot = 4;
RUA_CVAL char32_t enq = 5;
RUA_CVAL char32_t ack = 6;
RUA_CVAL char32_t bel = 7;
RUA_CVAL char32_t bs = 8;
RUA_CVAL char32_t ht = 9;
RUA_CVAL char32_t lf = 10;
RUA_CVAL char32_t vt = 11;
RUA_CVAL char32_t ff = 12;
RUA_CVAL char32_t cr = 13;
RUA_CVAL char32_t so = 14;
RUA_CVAL char32_t si = 15;
RUA_CVAL char32_t dle = 16;
RUA_CVAL char32_t dc1 = 17;
RUA_CVAL char32_t dc2 = 18;
RUA_CVAL char32_t dc3 = 19;
RUA_CVAL char32_t dc4 = 20;
RUA_CVAL char32_t nak = 21;
RUA_CVAL char32_t syn = 22;
RUA_CVAL char32_t etb = 23;
RUA_CVAL char32_t can = 24;
RUA_CVAL char32_t em = 25;
RUA_CVAL char32_t sub = 26;
RUA_CVAL char32_t esc = 27;
RUA_CVAL char32_t fs = 28;
RUA_CVAL char32_t gs = 29;
RUA_CVAL char32_t rs = 30;
RUA_CVAL char32_t us = 31;

RUA_CVAL char32_t del = 127;

// space

RUA_CVAL char32_t sp = 32;

//////////////////// C1 ////////////////////

// control

RUA_CVAL char32_t pad = 128;
RUA_CVAL char32_t hop = 129;
RUA_CVAL char32_t bph = 130;
RUA_CVAL char32_t nbh = 131;
RUA_CVAL char32_t ind = 132;
RUA_CVAL char32_t nel = 133;
RUA_CVAL char32_t ssa = 134;
RUA_CVAL char32_t esa = 135;
RUA_CVAL char32_t hts = 136;
RUA_CVAL char32_t htj = 137;
RUA_CVAL char32_t vts = 138;
RUA_CVAL char32_t pld = 139;
RUA_CVAL char32_t plu = 140;
RUA_CVAL char32_t ri = 141;
RUA_CVAL char32_t ss2 = 142;
RUA_CVAL char32_t ss3 = 143;
RUA_CVAL char32_t dcs = 144;
RUA_CVAL char32_t pu1 = 145;
RUA_CVAL char32_t pu2 = 146;
RUA_CVAL char32_t sts = 147;
RUA_CVAL char32_t cch = 148;
RUA_CVAL char32_t mw = 149;
RUA_CVAL char32_t spa = 150;
RUA_CVAL char32_t epa = 151;
RUA_CVAL char32_t sos = 152;
RUA_CVAL char32_t sgci = 153;
RUA_CVAL char32_t sci = 154;
RUA_CVAL char32_t csi = 155;
RUA_CVAL char32_t st = 156;
RUA_CVAL char32_t osc = 157;
RUA_CVAL char32_t pm = 158;
RUA_CVAL char32_t apc = 159;

///////////////// ISO-8859 /////////////////

// control

RUA_CVAL char32_t shy = 173;

// space

RUA_CVAL char32_t nbsp = 160;

////////////////// UNICODE //////////////////

// control

RUA_CVAL char32_t cgj = 847;

RUA_CVAL char32_t sam = 1807;

RUA_CVAL char32_t mvs = 6158;

RUA_CVAL char32_t zwsp = 8203;
RUA_CVAL char32_t zwnj = 8204;
RUA_CVAL char32_t zwj = 8205;
RUA_CVAL char32_t lrm = 8206;
RUA_CVAL char32_t rlm = 8207;
RUA_CVAL char32_t ls = 8232;
RUA_CVAL char32_t ps = 8233;
RUA_CVAL char32_t lre = 8234;
RUA_CVAL char32_t rle = 8235;
RUA_CVAL char32_t pdf = 8236;
RUA_CVAL char32_t lro = 8237;
RUA_CVAL char32_t rlo = 8238;

RUA_CVAL char32_t wj = 8288;
RUA_CVAL char32_t fa = 8289;
RUA_CVAL char32_t it = 8290;
RUA_CVAL char32_t is = 8291;
RUA_CVAL char32_t ip = 8292;

RUA_CVAL char32_t iss = 8298;
RUA_CVAL char32_t ass = 8299;
RUA_CVAL char32_t iafs = 8300;
RUA_CVAL char32_t aafs = 8301;
RUA_CVAL char32_t nads = 8302;
RUA_CVAL char32_t nods = 8303;

RUA_CVAL char32_t zwnbsp = 65279;

RUA_CVAL char32_t iaa = 65529;
RUA_CVAL char32_t ias = 65530;
RUA_CVAL char32_t iat = 65531;

// space

RUA_CVAL char32_t ospmk = 5760;

RUA_CVAL char32_t w1en = 8192;
RUA_CVAL char32_t w1em = 8193;
RUA_CVAL char32_t w1ensp = 8194;
RUA_CVAL char32_t w1emsp = 8195;
RUA_CVAL char32_t w3emsp = 8196;
RUA_CVAL char32_t w4emsp = 8197;
RUA_CVAL char32_t w6emsp = 8198;
RUA_CVAL char32_t w1mdsp = 8199;
RUA_CVAL char32_t w1npsp = 8200;
RUA_CVAL char32_t w1o5emsp = 8201;
RUA_CVAL char32_t w1o10emsp = 8202;

RUA_CVAL char32_t nnbsp = 8239;

RUA_CVAL char32_t mmsp = 8287;

RUA_CVAL char32_t w1cjksp = 12288;

} // namespace char_code

inline constexpr bool is_control(char32_t c) {
	return c < char_code::sp || c == char_code::del ||
		   (c >= char_code::nel && c <= char_code::apc) ||
		   c == char_code::shy || c == char_code::cgj || c == char_code::sam ||
		   c == char_code::mvs ||
		   (c >= char_code::zwsp && c <= char_code::rlo) ||
		   (c >= char_code::wj && c <= char_code::ip) ||
		   (c >= char_code::iss && c <= char_code::nods) ||
		   c == char_code::zwnbsp ||
		   (c >= char_code::iaa && c <= char_code::iat);
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

inline constexpr bool is_space(char32_t c) {
	return c == char_code::sp || is_control(c) || c == char_code::nbsp ||
		   c == char_code::ospmk ||
		   (c >= char_code::w1en && c <= char_code::w1o10emsp) ||
		   c == char_code::nnbsp || c == char_code::mmsp ||
		   c == char_code::w1cjksp;
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

RUA_CVAL const char *lf = "\n";
RUA_CVAL const char *crlf = "\r\n";
RUA_CVAL const char *cr = "\r";

#ifdef _WIN32

RUA_CVAL const char *sys_text = crlf;
RUA_CVAL const char *sys_con = crlf;

#elif defined(RUA_DARWIN)

RUA_CVAL const char *sys_text = cr;
RUA_CVAL const char *sys_con = lf;

#else

RUA_CVAL const char *sys_text = lf;
RUA_CVAL const char *sys_con = lf;

#endif

} // namespace eol

inline constexpr bool is_eol(char32_t c) {
	return c == char_code::lf || c == char_code::cr;
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
