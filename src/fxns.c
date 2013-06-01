/*-
 * Copyright (c) 2010 Ryan Kwolek
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of
 *     conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list
 *     of conditions and the following disclaimer in the documentation and/or other materials
 *     provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* 
 * fxns.c - 
 *    Misc. commonly used functions and CRT extensions
 */

#include "netgraph.h"
#include "fxns.h"

///////////////////////////////////////////////////////////////////////////////


int GetHostAttribsFromStr(const char *str) {
	int itmp = 0;

	while (*str) {
		switch (*str) {
			case 'b':
				itmp |= HA_BASTION;
				break;
			case 'd':
				itmp |= HA_DHCP;
				break;
			case 'e':
				itmp |= HA_SERVER;
				break;
			case 'f':
				itmp |= HA_FIREWALL;
				break;
			case 'l':
				itmp |= HA_LAPTOP;
				break;
			case 'n':  //reset all! cannot have other attribs if normal
				itmp = HA_NORMAL;
				break;
			case 'p':
				itmp |= HA_PC;
				break;
			case 'r':
				itmp |= HA_ROUTER;
				break;
			case 's':
				itmp |= HA_SWITCH;
				break;
			case 'v':
				itmp |= HA_VM;
				break;
			case 'u':
				itmp |= HA_UNMNGD;
				break;
			case 'g':
				itmp |= HA_BRIDGE;
				break;
			default:
				printf("WARNING: unknown host attribute \'%c\'\n", *str);
		}
		str++;
	}
	return itmp;
}


int GetConnAttribsFromStr(const char *str) {
	int itmp = 0;

	while (*str) {
		switch (*str) {
			case 't':
				itmp |= CA_10GBASETX;
				break;
			case 'g':
				itmp |= CA_1GBASETX;
				break;
			case 'f':
				itmp |= CA_100BASETX;
				break;
			case 'e':
				itmp |= CA_10BASET;
				break;
			case 'v':
				itmp |= CA_VIRTUAL;
				break;
			case 'w':
				itmp |= CA_WIRELESS;
				break;
			default:
				printf("WARNING: unknown connection attribute \'%c\'\n", *str);
		}
		str++;
	}
	return itmp;
}


int GetHostColor(int attribs) {
	int ci;

	if (attribs == HA_NORMAL) {
		ci = CLR_WHITE + LIGHT;
		hostattribsused |= 0x80000000;
	} else {
		if (attribs & HA_PC) {
			hostattribsused |= HA_PC;
			ci = CLR_BLUE + LIGHT;
		}
		if (attribs & HA_LAPTOP) {
			hostattribsused |= HA_LAPTOP;			
			ci = CLR_AQUA + LIGHT;
		}
		if (attribs & HA_SERVER) {
			hostattribsused |= HA_SERVER;			
			ci = CLR_GREEN + LIGHT;
		}
		if (attribs & HA_VM) {
			hostattribsused |= HA_VM;
			ci = CLR_PURPLE + LIGHT;
		}
		if (attribs & HA_FIREWALL) {
			hostattribsused |= HA_FIREWALL;
			ci = CLR_RED + LIGHT;
		}
		if (attribs & HA_SWITCH) {
			hostattribsused |= HA_SWITCH;			
			ci = CLR_YELLOW + LIGHT;
		}
		if (attribs & HA_ROUTER) {
			hostattribsused |= HA_ROUTER;			
			ci = CLR_ORANGE + LIGHT;
		}
	}
	return ci;
}


int GetConnColor(int attribs) {
	int ci;

	if (attribs & CA_10GBASETX) {
		connattribsused |= CA_10GBASETX;			
		ci = CLR_RED;
	}
	if (attribs & CA_1GBASETX) {
		connattribsused |= CA_1GBASETX;
		ci = CLR_ORANGE;
	}
	if (attribs & CA_100BASETX) {
		connattribsused |= CA_100BASETX;
		ci = CLR_YELLOW;
	}
	if (attribs & CA_10BASET) {
		connattribsused |= CA_10BASET;
		ci = CLR_GREEN;
	}
	if (attribs & CA_VIRTUAL) {
		connattribsused |= CA_VIRTUAL;
		ci = CLR_PURPLE;
	}
	if (attribs & CA_WIRELESS) {
		connattribsused |= CA_WIRELESS;
		ci = CLR_BLUE;
	}
	return ci;
}


///////////////////////////////////////////////////////////////////////////////


void lcase(char *str) {
	while (*str) {
		*str = tolower(*str);
		str++;
	}
}


char *skipws(char *text) {
	while (*text == ' ' || *text == '\t')
		text++;
	return text;
}


char *findws(char *text) {
	while (*text != ' ' && *text != '\t') {
		if (!*text)
			return NULL;
		text++;
	}
	return text;
}


int popcount(int n) {
	int i, count;

	count = 0;
	for (i = 0; i != sizeof(n) * 8; i++) {
		if (n & (1 << i))
			count++;
	}
	return count;
}

