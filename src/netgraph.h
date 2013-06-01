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

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define _CRT_SECURE_NO_WARNINGS
	#include <windows.h>
	#include <winsock2.h>

	typedef __int8 int8_t;
	typedef __int16 int16_t;
	typedef __int32 int32_t;
	typedef __int64 int64_t;
	typedef unsigned __int8 uint8_t;
	typedef unsigned __int16 uint16_t;
	typedef unsigned __int32 uint32_t;
	typedef unsigned __int64 uint64_t;
#else
	#include <stdint.h>
	#include <string.h>
	#include <errno.h>
	#include <ctype.h>
	#include <arpa/inet.h>

	#define MAX_PATH 256
#endif
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <gd.h>
#include <gdfontt.h>
#include <gdfonts.h>
#include <gdfontmb.h>
#include <gdfontl.h>

#define HELP_MSG "usage: netgraph [-hvV] <filename>"
#define VER_MSG "---------------\nnetgraph v1.0\n-------------------"

#define PI (3.14159)

#define HA_NORMAL	0x00000000 //white
#define HA_FIREWALL 0x00000001 //red
#define HA_ROUTER	0x00000002 //orange
#define HA_SWITCH	0x00000004 //yellow
#define HA_VM		0x00000008 //purple
#define HA_SERVER	0x00000010 //green
#define HA_PC		0x00000020 //blue
#define HA_LAPTOP	0x00000040 //dunno
#define HA_BASTION	0x00000080
#define HA_DHCP		0x00000100
#define HA_UNMNGD	0x00000200
#define HA_BRIDGE	0x00000400
#define NODE_UNRESOLVED 0x80000000

#define CA_10BASET		0x001
#define CA_100BASETX	0x002
#define CA_1GBASETX		0x004
#define CA_10GBASETX	0x008
#define CA_VIRTUAL		0x010
#define CA_WIRELESS		0x020

#define TL_HOSTS		0xFF
#define TL_OSES			0x3F

#define RECT_CX 120
#define RECT_CY  60
#define RECT_BUFSPACE_X 20
#define RECT_BUFSPACE_Y 30

#define DRAW_ORIGIN 3

#define CIRC_POINTS	12

#define CLR_WHITE		0
#define CLR_BLACK		1
#define CLR_RED			2
#define CLR_ORANGE		3
#define CLR_YELLOW		4
#define CLR_GREEN		5
#define CLR_BLUE		6
#define CLR_PURPLE		7
#define CLR_AQUA		8
#define NUM_COLORS   (sizeof(colordescs) / sizeof(colordescs[0]))
#define NUM_HOSTCLRS (sizeof(hostcolors) / sizeof(hostcolors[0]))
#define NUM_CONNCLRS (sizeof(conncolors) / sizeof(conncolors[0]))
#define LIGHT (NUM_COLORS)

#define DESC_IP	   0
#define DESC_OS    1
#define DESC_MODEL 2
#define DESC_DESCR 3
#define DESC_FLAGS 4
#define DESC_CONNS 5

typedef struct _chain {
	int numentries;
	void *entry[1];
} CHAIN, *LPCHAIN;

typedef struct _node {
	int value;
	void *data;
	struct _node *lchild;
	struct _node *rchild;
} NODE, *LPNODE;

typedef struct _osicon {
	char name[32];
	gdImagePtr icon;
} OS, *LPOS;

struct __host;

typedef struct _conn {
	struct _host *connto;
	int attribs;
} CONN, *LPCONN;

typedef struct _host {
	char name[32];
	//union {
	//	struct in_addr ipv4;
	//	struct in6_addr ipv6;
	//}
	char ip[16];
	LPOS os;
	char model[64];
	char descr[64];
	int attribs;
	short listnum;
	short drawnum;
	short depth;
	short orientation;
	short x;
	short y;
	struct _host *parent;
	int nconns;
	int size_connalloc;
	LPCONN conns;
} HOST, *LPHOST;

typedef struct _color {
	const char *name;
	unsigned char r;
	unsigned char g;
	unsigned char b;
} COLORDESC, *LPCOLORDESC;

extern int hostattribsused;
extern int connattribsused;
extern COLORDESC colordescs[9];
extern int hostcolors[8];
extern int conncolors[6];

void HandleCmdArgs(int argc, char *argv[]);
int ParseDescFile(const char *descfile);
void SaveToPng(const char *filename, gdImagePtr im);

LPHOST *PrioQueueCreateFromHT(LPCHAIN *ht, unsigned int htlen, int nitems);
int HostArrayCompare(const void *e1, const void *e2);

void PlotNetwork(LPHOST *hostarr, int nitems);
void ArrangeNetwork(LPHOST *hostarr, int nitems);
gdImagePtr CreateBaseImage(gdImagePtr *bg, const char *bgfilename);
void AllocateColors(gdImagePtr im);
void DrawNetwork(gdImagePtr im, gdImagePtr bkgnd, LPHOST *hostarr, int nitems);
void DrawStaticItems(gdImagePtr im);

//void GetStartingCoords(int pos_tick, int *x, int *y, int center_x, int center_y);
void AdvanceCoordsRect(short *x, short *y, short node_depth, short *ndrawn, short *orientation,
					   int clockwise, int origin_x, int origin_y);
void AdvanceCoordsCirc(short *x, short *y, short node_depth, short *ndrawn, short *orientation,
					   int clockwise, int origin_x, int origin_y);

