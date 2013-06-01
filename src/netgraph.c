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
 * netgraph.c - 
 *    Main source file for netgraph, containing main() and mostly everything
 */


#include "netgraph.h"
#include "hashtable.h"
#include "icons.h"
#include "fxns.h"

char netdescfile[256];
int verbose;
int nhtitems;
LPCHAIN hosts[TL_HOSTS + 1];
LPHOST *hosts_array;
int img_cx, img_cy;
int draw_rect;

const char *cfgstrs[] = {
	"ip",
	"os",
	"model",
	"descr",
	"flags",
	"conns"
};

COLORDESC colordescs[] = {
	{"white",	 0xFF, 0xFF, 0xFF},
	{"black",	 0x00, 0x00, 0x00},
	{"red",		 0xFF, 0x00, 0x00},
	{"orange",	 0xFF, 0x80, 0x00},
	{"yellow",	 0xFF, 0xFF, 0x00},
	{"green",	 0x00, 0xFF, 0x00},
	{"blue",	 0x00, 0x00, 0xFF},
	{"purple",   0xFF, 0x00, 0xFF},
	{"aqua",     0x00, 0xFF, 0xFF}
};

int hostcolors[] = {
	CLR_WHITE + LIGHT,
	CLR_RED + LIGHT,
	CLR_ORANGE + LIGHT,
	CLR_YELLOW + LIGHT,
	CLR_PURPLE + LIGHT,
	CLR_GREEN + LIGHT,
	CLR_BLUE + LIGHT,
	CLR_AQUA + LIGHT
};

int conncolors[] = {
	CLR_GREEN,
	CLR_YELLOW,
	CLR_ORANGE,
	CLR_RED,
	CLR_PURPLE,
	CLR_BLUE
};

const char *hostattribstrs[] = {
	"No attribs",
	"Firewall",
	"Router",
	"Switch",
	"VM",
	"Server",
	"Workstation",
	"Laptop"
};

const char *connattribstrs[] = {
	"10BaseT",
	"100BaseTX",
	"1GBaseTX",
	"10GBaseTX",
	"Virtual",
	"Wireless"
};

int colors[NUM_COLORS * 2];
int hostattribsused;
int connattribsused;


//TODO:
// no duplicate ips
// get the clockwise/counterclockwise thing working
// lighten the background colors
// add real images instead of boxes, maybe.
// add handling (repositioning) of bastion host attribute 

///////////////////////////////////////////////////////////////////////////////


void main(int argc, char *argv[]) {
	int i;
	gdImagePtr im, bg;

	draw_rect = 1;
	HandleCmdArgs(argc, argv);

	LoadOSIcons();

	if (!ParseDescFile(netdescfile)) {
		puts("Failed to parse description file!");
		return;
	}

	hosts_array = PrioQueueCreateFromHT(hosts, TL_HOSTS, nhtitems);
	for (i = 0; i != nhtitems; i++)
		hosts_array[i]->listnum = i;
	PlotNetwork(hosts_array, nhtitems);
	ArrangeNetwork(hosts_array, nhtitems);

	im = CreateBaseImage(&bg, "bkgnd.jpg");
	AllocateColors(im);
	DrawNetwork(im, bg, hosts_array, nhtitems);
	DrawStaticItems(im);
	SaveToPng("test.png", im);
	gdImageDestroy(im);
}


void HandleCmdArgs(int argc, char *argv[]) {
	int i, slen;

	for (i = 1; i != argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
				case 'h':
					puts(HELP_MSG);
					exit(0);
					return;
				case 'v':
					verbose = 1;
					break;
				case 'V':
					puts(VER_MSG);
					exit(0);
					return;
				default:
					printf("ignored invalid switch \'%c\'.", argv[i][1]);
			}
		} else {
			if (i == (argc - 1)) {
				strncpy(netdescfile, argv[i], sizeof(netdescfile));
				netdescfile[sizeof(netdescfile) - 1] = 0;
			}
		}
	}

	if (argc == 1) {
		puts("Filename of network description:");
		fgets(netdescfile, sizeof(netdescfile), stdin);
		slen = strlen(netdescfile);
		if (netdescfile[slen - 1] == '\n')
			netdescfile[slen - 1] = 0;
	}
}


/*
network description file format:

host attribs:
	b - bastion host
	d - dhcp
	f - firewall
	e - server
	l - laptop
	p - pc
	r - router
	s - switch
	v - virtual machine
	n - normal (no attribs)
	
connection attribs:
	v - virtual
	w - wireless
	e - 10BaseT
	f - 100BaseTX
	g - 1GBaseTX
	t - 10GBaseTX

for each record:
 [name of host] [host ip] [host OS] [host attribute]
 for each connection:
  [connected to (name)] [connection attribute]
*/


int ParseDescFile(const char *descfile) {
	char asdf[256], tmp[32], *sdfg, *dfgh, *fghj;
	LPHOST host, thost;
	LPOS lpos;
	FILE *file;
	int nentries, lines, slen, i;

	nentries = 0;
	lines    = 0;

	file = fopen(descfile, "r");
	if (!file) {
		printf("failed to open file %s, errno: %d\n", descfile, errno);
		return 0;
	}
	
	while (!feof(file)) {
		lines++;
		fgets(asdf, sizeof(asdf), file);
		if (*asdf == '#')
			continue;
		slen = strlen(asdf);
		if (slen >= 1 && asdf[slen - 1] == '\n')
			asdf[slen - 1] = 0;
		if (slen >= 2 && asdf[slen - 2] == '\r')
			asdf[slen - 2] = 0;

		sdfg = skipws(asdf);
		if (*sdfg == '}')
			host = NULL;

		if (*(uint32_t *)sdfg == 'tsoh') {
			sdfg += 5;
			dfgh = strchr(sdfg, '{');
			if (!dfgh) {
				sdfg -= 5;
				goto actually_a_specifier;
			}
			if (dfgh - sdfg <= 1)
				continue;
			dfgh--;
			*dfgh = 0;

			host = HtGetValue(sdfg, hosts, TL_HOSTS);
			if (!host) {
				host = (LPHOST)malloc(sizeof(HOST));
				memset(host, 0, sizeof(HOST));
				strncpy(host->name, sdfg, sizeof(host->name));
				host->name[sizeof(host->name) - 1] = 0;
				HtInsertValue(host->name, host, hosts, TL_HOSTS);
			} else {
				if (host->attribs != NODE_UNRESOLVED) {
					printf("WARNING: Duplicate hostname \'%s\' on line %d\n",
						host->name, lines);
				} else {
					if (verbose)
						printf("Forward declaration of %s resolved.\n", sdfg);
				}
			}

			nentries++;
		} else {
			actually_a_specifier:
			if (!host)
				continue;
			dfgh = sdfg;
			fghj = findws(sdfg);
			sdfg = strchr(sdfg, '=');
			if (!sdfg)
				continue;
			*sdfg = 0;
			sdfg = skipws(++sdfg);
			if (fghj)
				*fghj = 0;

			for (i = 0; i != sizeof(cfgstrs) / sizeof(cfgstrs[0]); i++) {
				if (!strcmp(dfgh, cfgstrs[i]))
					break;
			}
			if (i == sizeof(cfgstrs) / sizeof(cfgstrs[0]))
				continue;
			switch (i) {
				case DESC_IP:
					strncpy(host->ip, sdfg, sizeof(host->ip));
					host->ip[sizeof(host->ip) - 1] = 0;
					break;
				case DESC_OS:
					strncpy(tmp, sdfg, sizeof(tmp));
					tmp[sizeof(tmp) - 1] = 0;
					lcase(tmp);
					lpos = HtGetValue(tmp, oses, TL_OSES);
					if (!lpos) {
						lpos = malloc(sizeof(OS));
						strcpy(lpos->name, tmp);
						lpos->icon = NULL;
						numoses++;
					}
					host->os = lpos;
					break;
				case DESC_MODEL:
					strncpy(host->model, sdfg, sizeof(host->model));
					host->model[sizeof(host->model) - 1] = 0;
					break;
				case DESC_DESCR:
					/*dfgh = sdfg;
					while (*dfgh) {
						if (*dfgh == '\\' && *(dfgh + 1) == 'n') {
							*dfgh = ' ';
							*(dfgh + 1) = '\n';
						}
						dfgh++;
					}*/
					strncpy(host->descr, sdfg, sizeof(host->descr));
					host->descr[sizeof(host->descr) - 1] = 0;
					break;
				case DESC_FLAGS:
					host->attribs = GetHostAttribsFromStr(sdfg);
					break;
				case DESC_CONNS:
					if (!host->conns) {
						host->nconns = 0;
						host->conns  = malloc(sizeof(CONN));
						host->size_connalloc = 1;
					}
					
					sdfg = strtok(sdfg, " ");
					dfgh = strtok(NULL, " ");
					while (sdfg && dfgh) {
						thost = HtGetValue(sdfg, hosts, TL_HOSTS);
						if (!thost) {
							thost = (LPHOST)malloc(sizeof(HOST));
							memset(thost, 0, sizeof(HOST));
							strncpy(thost->name, sdfg, sizeof(thost->name));
							thost->name[sizeof(thost->name) - 1] = 0;
							HtInsertValue(thost->name, thost, hosts, TL_HOSTS);
							if (verbose)
								printf("Forward declaration of %s made.\n", sdfg);
							thost->attribs = NODE_UNRESOLVED;
						}
						host->conns[host->nconns].connto = thost;
						host->nconns++;
						if (host->nconns == host->size_connalloc) {
							host->size_connalloc <<= 1;
							host->conns = realloc(host->conns, sizeof(CONN) * host->size_connalloc);
						}
						host->conns[host->nconns - 1].attribs = GetConnAttribsFromStr(dfgh);
						
						sdfg  = strtok(NULL, " ");
						dfgh = strtok(NULL, " ");
					}
			}
		}
	}
	
	nhtitems = nentries;

	fclose(file);
	return 1;
}


void SaveToPng(const char *filename, gdImagePtr im) {
	FILE *out;
	int size;
	char *data;

	out = fopen(filename, "wb");
	if (!out) {
		printf("Couldn't open %s for writing, errno: %d\n", filename, errno);
		return;
	}

	data = (char *)gdImagePngPtr(im, &size);

	fwrite(data, 1, size, out);

	fclose(out);
	gdFree(data);
}


LPHOST *PrioQueueCreateFromHT(LPCHAIN *ht, unsigned int htlen, int nitems) {
	int i, j, k;
	LPHOST *hostarr;

	k = 0;
	hostarr = (LPHOST *)malloc(nitems * sizeof(LPHOST));

	for (i = 0; i != htlen; i++) {
		if (ht[i]) {
			for (j = 0; j != ht[i]->numentries; j++) {
				hostarr[k] = (LPHOST)ht[i]->entry[j];
				k++;	
			}
		}
	}

	qsort(hostarr, nitems, sizeof(LPHOST), HostArrayCompare);

	return hostarr;
}


int HostArrayCompare(const void *e1, const void *e2) {
	int diff;
	
	diff = (*(LPHOST *)e2)->nconns - (*(LPHOST *)e1)->nconns;
	if (diff)
		return diff;
	return inet_addr((*(LPHOST *)e2)->ip) - inet_addr((*(LPHOST *)e1)->ip);
}


int ConnArrayCompare(const void *e1, const void *e2) {
	return ((LPCONN)e1)->connto->drawnum - ((LPCONN)e2)->connto->drawnum;
}


int GetTotalDepth(LPHOST *hostarr, int nitems) {
	int i, curdepth;

	curdepth = 1;
	for (i = 0; i != nitems - 1; i++) {
		if (hostarr[i]->nconns > hostarr[i + 1]->nconns)
			curdepth++;
	}
	return curdepth;
}


void PlotNetwork(LPHOST *hostarr, int nitems) {
	short pos_x, pos_y;
	short ndrawn, orientation, node_depth;
	int ncurdepthconns, i, j;
	LPHOST lphost, lpconn;

	if (!hostarr)
		return; //add status code?

	//find the dimensions of the image to create
	// hrmm.. maybe it could be based on the number of nodes / 4
	// cool for a square, but make the aspect ratio adjustable
	i = GetTotalDepth(hostarr, nitems);
	img_cx = i * 2 * (RECT_CX + RECT_BUFSPACE_X);
	img_cy = i * 2 * (RECT_CY + RECT_BUFSPACE_Y);
	printf("total depth: %d\n", i);
	//img_cx = ((nitems / 2) * RECT_CX) + 200;
	//img_cy = ((nitems / 2) * RECT_CY) + 200;
	
	node_depth  = 0;
	ndrawn		= 0;
	orientation = 0;

	pos_x = img_cx >> 1;
	pos_y = img_cy >> 1;

	if (!nitems)
		return;

	hostarr[0]->x = pos_x;
	hostarr[0]->y = pos_y;
	hostarr[0]->drawnum		= 0;
	hostarr[0]->orientation = 0;
	hostarr[0]->depth  = 0;//////the first node need not be of 0 depth.... hmmm kludgy
	for (i = 1; i != nitems; i++) {
		lphost = hostarr[i];
		if (i && (lphost->nconns < hostarr[i - 1]->nconns)) {
			ncurdepthconns = 1;
			j = i;
			///same problem, i assume it's square
			while ((j != nitems - 1) && (hostarr[i]->nconns == hostarr[i + 1]->nconns)) {
				ncurdepthconns++;
				j++;
			}
			if (draw_rect) {
				do {
					node_depth++; ////uhh... fix for non squares
				} while (ncurdepthconns > (node_depth << 3));
			} else {
				node_depth++; ///hrmmm
			}
			if (verbose)
				printf("new depth (%d) on %dth item\n", node_depth, i);
			ndrawn = 0;
		}
		
		hostarr[i]->drawnum = ndrawn;
		hostarr[i]->orientation = orientation;
		hostarr[i]->depth = node_depth;

		if (draw_rect)
			AdvanceCoordsRect(&pos_x, &pos_y, node_depth, &ndrawn, &orientation, 0, img_cx >> 1, img_cy >> 1);
		else
			AdvanceCoordsCirc(&pos_x, &pos_y, node_depth, &ndrawn, &orientation, 0, img_cx >> 1, img_cy >> 1);

		hostarr[i]->x = pos_x;
		hostarr[i]->y = pos_y;	
		
		for (j = 0; j != lphost->nconns; j++) {
			lpconn = lphost->conns[j].connto;
			if (lpconn->nconns > lphost->nconns) {
				if (!lphost->parent || (lpconn->nconns > lphost->parent->nconns))
					lphost->parent = lpconn;
			}
		}
	}
}


void ArrangeNetwork(LPHOST *hostarr, int nitems) {
	int i, j, k;
	LPHOST lphost, lpconn, lowest_drawn;

	for (i = 0; i != nitems; i++) {
		lphost = hostarr[i];
		if (lphost->nconns > 1) {
			//sort the conns first according to drawnum!
			qsort(lphost->conns, lphost->nconns, sizeof(CONN), ConnArrayCompare);

			//fix gaps in between conn runs
			k = lphost->conns[0].connto->drawnum; //same thing here! crapola
			for (j = 0; j != lphost->nconns; j++) {
				lpconn = lphost->conns[j].connto;
				if (lpconn->drawnum != k) { //one of them is left out!
					//swap with the last one in the range and fix the drawnum too
				}
				k++;
			}

			//it only has one parent, so the lowest must be either 0 or 1
			lowest_drawn = (lphost->conns[0].connto == lphost->parent) ?
				lphost->conns[1].connto : lphost->conns[0].connto; 
			if (abs(lowest_drawn->drawnum + (lphost->nconns >> 1) - lphost->drawnum) > (lphost->nconns >> 1)) { //fix for orientation here!
				lphost->drawnum = (lowest_drawn->drawnum + (lphost->nconns >> 1)) >> (lowest_drawn->depth - lphost->depth);
				//	- ((lowest_drawn->depth - lphost->depth) << 1);
				lphost->orientation = (lphost->drawnum / (lphost->depth << 1)) & 3;
				AdvanceCoordsRect(&lphost->x, &lphost->y, lphost->depth,
					&lphost->drawnum, &lphost->orientation, 0, img_cx >> 1, img_cy >> 1);
				lphost->drawnum--;
			}

		}
	}
}


gdImagePtr CreateBaseImage(gdImagePtr *bg, const char *bgfilename) {
	gdImagePtr im;

	if (!bg)
		return NULL;

	*bg = LoadImageGd(bgfilename);
	im = gdImageCreateTrueColor(img_cx, img_cy);

	if (*bg)
		gdImageCopyResampled(im, *bg, 0, 0, 0, 0, im->sx, im->sy, (*bg)->sx, (*bg)->sy);
	//else
		//gdImageFill(im, 0, 0, bkgndclr);

	return im;
}


void AllocateColors(gdImagePtr im) {
	int i, r, g, b;

	for (i = 0; i != NUM_COLORS; i++)
		colors[i] = gdImageColorAllocate(im, colordescs[i].r, colordescs[i].g, colordescs[i].b);
	for (i = 0; i != NUM_COLORS; i++) {
		r = colordescs[i].r;
		if (!r)
			r = 0x90;
		else
			r += 0x30;
		if (r > 0xFF)
			r = 0xFF;
		g = colordescs[i].g;
		if (!g)
			g = 0x90;
		else
			g += 0x30;
		if (g > 0xFF)
			g = 0xFF;
		b = colordescs[i].b;
		if (!b)
			b = 0x90;
		else
			b += 0x30;
		if (b > 0xFF)
			b = 0xFF;
		colors[i + NUM_COLORS] = gdTrueColorAlpha(r, g, b, 170);
	}
}


void DrawNetwork(gdImagePtr im, gdImagePtr bkgnd, LPHOST *hostarr, int nitems) {
	int i, k, ci, newx, j;
	int left, right, top, bottom;
	char sdfg[128], *asdf, *dfgh;
	float fx, fy;
	LPHOST lphost;

	fx = (float)bkgnd->sx / (float)img_cx;
	fy = (float)bkgnd->sy / (float)img_cy;

	for (i = 0; i != nitems; i++) {
		lphost = hostarr[i];
		for (k = 0; k != lphost->nconns; k++) {
			ci = GetConnColor(lphost->conns[k].attribs);

			gdImageSetAntiAliased(im, colors[CLR_BLACK]);
			gdImageSetThickness(im, 8);
			gdImageLine(im, lphost->x, lphost->y, lphost->conns[k].connto->x,
				lphost->conns[k].connto->y, gdAntiAliased);
			gdImageSetAntiAliased(im, colors[ci]);
			gdImageSetThickness(im, 4);
			gdImageLine(im, lphost->x, lphost->y, lphost->conns[k].connto->x,
				lphost->conns[k].connto->y, gdAntiAliased); 
		}		
	}
	gdImageSetThickness(im, 1);
	for (i = 0; i != nitems; i++) {
		lphost = hostarr[i];

		left   = lphost->x - (RECT_CX / 2);
		right  = lphost->x + (RECT_CX / 2);
		top    = lphost->y - (RECT_CY / 2);
		bottom = lphost->y + (RECT_CY / 2);

		ci = GetHostColor(lphost->attribs);
		
		gdImageCopyResampled(im, bkgnd, left, top, 
			(int)((float)left * fx), (int)((float)top * fy),
			RECT_CX, RECT_CY,
			(int)((float)RECT_CX * fx), (int)((float)RECT_CY * fy));
		gdImageFilledRectangle(im, left, top, right, bottom, colors[ci]);
		gdImageRectangle(im, left, top, right, bottom, colors[CLR_BLACK]);
		
		if (lphost->attribs & HA_UNMNGD) {
			gdImageString(im, gdFontGetSmall(), left + 5, top + 3, (unsigned char *)lphost->model, colors[CLR_BLACK]);
			sprintf(sdfg, "Unmanaged %s", (lphost->attribs & HA_SWITCH) ? "switch" :
				((lphost->attribs & HA_FIREWALL) ? "firewall" : ""));
			gdImageString(im, gdFontGetTiny(), left + 5, top + 16, (unsigned char *)sdfg, colors[CLR_BLACK]);
			//sprintf(sdfg, "drawnum: %d", lphost->drawnum);
			gdImageString(im, gdFontGetTiny(), left + 5, bottom - 20, (unsigned char *)lphost->descr, colors[CLR_BLACK]);
		} else {
			gdImageString(im, gdFontGetMediumBold(), left + 5, top + 3, (unsigned char *)lphost->name, colors[CLR_BLACK]);
			//sprintf(sdfg, "drawnum: %d", lphost->drawnum); //sprintf(sdfg, "OS: %s", lphost->os); 
			gdImageString(im, gdFontGetTiny(), left + 5, top + 16, (unsigned char *)lphost->ip, colors[CLR_BLACK]);
			gdImageString(im, gdFontGetTiny(), left + 5, top + 25, (unsigned char *)lphost->model, colors[CLR_BLACK]);

			j = 0;
			dfgh = asdf = lphost->descr;
			while (*asdf) {
				if (*asdf == '\\' && *(asdf + 1) == 'n') {
					*asdf = 0;
					gdImageString(im, gdFontGetTiny(), left + 5, bottom - 26 + j * 9, (unsigned char *)dfgh, colors[CLR_BLACK]);
					asdf += 2;
					dfgh = asdf;
					j++;
				}
				asdf++;
			}
			gdImageString(im, gdFontGetTiny(), left + 5, bottom - 26 + j * 9, (unsigned char *)dfgh, colors[CLR_BLACK]);

			if (lphost->os && lphost->os->icon) {
				newx = (int)((float)lphost->os->icon->sx * (24.f / (float)lphost->os->icon->sy));
				gdImageCopyResampled(im, lphost->os->icon, right - newx, top + 1,
					0, 0, newx, 24, lphost->os->icon->sx, lphost->os->icon->sy);
			}
		}
	}

	gdImageDestroy(bkgnd);
}





void AdvanceCoordsRect(short *x, short *y, short node_depth, short *ndrawn,
					   short *orientation, int clockwise, int origin_x, int origin_y) {
	int drawpos;
	/* clockwise draw  2, 1, 6, 5
		 001
		|---|
	110 |   | 010
		|---|
		 101          */
	/* counterclockwise draw  6, 5, 2, 1
		 101
		|---|
	010 |   | 110
		|---|
		 001         */

	if (!x || !y || !orientation || !ndrawn)
		return;

	if (!node_depth) {
		*x += RECT_CX + RECT_BUFSPACE_X;
		return;
	}

	drawpos = *ndrawn % (node_depth << 1);
	switch (*orientation) {
		case 0:	//bottom
			*x = origin_x - (node_depth * (RECT_CX + RECT_BUFSPACE_X))
				+ (drawpos * (RECT_CX + RECT_BUFSPACE_X));
			*y = origin_y + (node_depth * (RECT_CY + RECT_BUFSPACE_Y));
			break;
		case 1: //right
			*x = origin_x + (node_depth * (RECT_CX + RECT_BUFSPACE_X));
			*y = origin_y + (node_depth * (RECT_CY + RECT_BUFSPACE_Y))
				- (drawpos * (RECT_CY + RECT_BUFSPACE_Y));
			break;
		case 2: //top
			*x = origin_x + (node_depth * (RECT_CX + RECT_BUFSPACE_X))
				- (drawpos * (RECT_CX + RECT_BUFSPACE_X));
			*y = origin_y - (node_depth * (RECT_CY + RECT_BUFSPACE_Y));
			break;
		case 3: //left
			*x = origin_x - (node_depth * (RECT_CX + RECT_BUFSPACE_X));
			*y = origin_y - (node_depth * (RECT_CY + RECT_BUFSPACE_Y))
				+ (drawpos * (RECT_CY + RECT_BUFSPACE_Y));		
	}
	if (verbose)
		printf("ndrawn: %d | new x: %d, y: %d\n", *ndrawn, *x, *y);

	(*ndrawn)++;
	if (*ndrawn % (node_depth << 1) == 0) {
		(*orientation) -= (clockwise << 1) - 1;
		*orientation &= 3;
		if (verbose)
			printf("orientation: %d\n", *orientation);
	}

	/*o = *orientation;
	if (*ndrawn == ((node_depth << 1))) {
		//if (clockwise)
		//	o = (~o & 4) | (((o & 3) + 1) & 3);
		//else
		//	o = (~o & 4) | (((o & 3) - 1) & 3);
		o = ((o + 4) & 12) | (~o & 3);
		*ndrawn = 0;
		printf("rotation: %d\n", o);
	}
	*x += -(RECT_CX + RECT_BUFSPACE_X) * (1 - ((o & 8) >> 2)) * (o & 1); 
	*y += -(RECT_CY + RECT_BUFSPACE_Y) * (1 - ((o & 8) >> 2)) * ((o & 2) >> 1); 
	printf("incremented by: x: %d, y: %d\n",
		-(RECT_CX + RECT_BUFSPACE_X) * (1 - ((o & 4) >> 1)) * (o & 1),
		-(RECT_CY + RECT_BUFSPACE_Y) * (1 - ((o & 4) >> 1)) * (o & 2)); 
	(*ndrawn)++;
	*orientation = o;*/
}


void AdvanceCoordsCirc(short *x, short *y, short node_depth, short *ndrawn,
					   short *orientation, int clockwise, int origin_x, int origin_y) {
	double angle;

	if (!x || !y || !orientation || !ndrawn)
		return;

	angle = ((double)*ndrawn * 2.f * PI) / (double)CIRC_POINTS;
	*x =  (short)(((double)(node_depth * (RECT_CX + RECT_BUFSPACE_X))) * cos(angle)) + origin_x;
	*y =  (short)(((double)(node_depth * (RECT_CY + RECT_BUFSPACE_Y))) * sin(angle)) + origin_y;
	(*ndrawn)++;
	if (verbose)
		printf("ndrawn: %d | new x: %d, y: %d\n", *ndrawn, *x, *y);
}


/***********
 *6	  7   0*
 *		   *
 *5		  1*
 *		   *
 *4   3   2*
 ***********/
/*void GetStartingCoordsRect(int pos_tick, int *x, int *y, int center_x, int center_y) {
	if (!x || !y) 
		return;

	switch (pos_tick) {
		case 0:
			*x = center_x + RECT_CX + RECT_BUFSPACE_X;
			*y = center_y + RECT_CY + RECT_BUFSPACE_Y;
			break;
		case 1:
			*x = center_x + RECT_CX + RECT_BUFSPACE_X;
			*y = center_y;
			break;
		case 2:
			*x = center_x + RECT_CX + RECT_BUFSPACE_X;
			*y = center_y - RECT_CY - RECT_BUFSPACE_Y;
			break;
		case 3:
			*x = center_x;
			*y = center_y - RECT_CY - RECT_BUFSPACE_Y;
			break;
		case 4:
			*x = center_x - RECT_CX - RECT_BUFSPACE_X;
			*y = center_y - RECT_CY - RECT_BUFSPACE_Y;
			break;
		case 5:
			*x = center_x - RECT_CX - RECT_BUFSPACE_X;
			*y = center_y;
			break;
		case 6:
			*x = center_x - RECT_CX - RECT_BUFSPACE_X;
			*y = center_y + RECT_CY + RECT_BUFSPACE_Y;
			break;
		case 7:
			*x = center_x;
			*y = center_y + RECT_CY + RECT_BUFSPACE_Y;
			break;
		default:
			*x = 0;
			*y = 0;
	}

}*/


void DrawStaticItems(gdImagePtr im) {
	int i, ndrawn, hpop, cpop;

	i = 0;
	ndrawn = 0;
	hpop = popcount(hostattribsused &
		(HA_FIREWALL | HA_ROUTER | HA_SWITCH |
		HA_VM | HA_SERVER | HA_PC | HA_LAPTOP));
	cpop = popcount(connattribsused &
		(CA_10BASET | CA_100BASETX | CA_1GBASETX |
		CA_10GBASETX | CA_VIRTUAL | CA_WIRELESS));

	if (hostattribsused & 0x80000000) {
		hpop++;
		gdImageFilledRectangle(im, img_cx - 65,
			img_cy - (cpop * 13) - (hpop * 29) - 16 + ndrawn * 29,
			img_cx - 7, img_cy - (cpop * 13) - (hpop * 29) + ndrawn * 29,
			colors[hostcolors[i]]);
		gdImageString(im, gdFontGetTiny(), img_cx - 62,
			img_cy - (cpop * 13) - (hpop * 29) - 16 + ndrawn * 29 + 4,
			(unsigned char *)hostattribstrs[i], colors[CLR_BLACK]);
		ndrawn++;
	}
	for (i = 1; i != NUM_HOSTCLRS; i++) {
		if (hostattribsused & (1 << (i - 1))) {
			gdImageFilledRectangle(im, img_cx - 65,
				img_cy - (cpop * 13) - (hpop * 29) - 16 + ndrawn * 29,
				img_cx - 7, img_cy - (cpop * 13) - (hpop * 29) + ndrawn * 29,
				colors[hostcolors[i]]);
			gdImageString(im, gdFontGetTiny(), img_cx - 62,
				img_cy - (cpop * 13) - (hpop * 29) - 16 + ndrawn * 29 + 4,
				(unsigned char *)hostattribstrs[i], colors[CLR_BLACK]);
			ndrawn++;
		}
	}

	ndrawn = 0;
	gdImageSetThickness(im, 3);
	for (i = 0; i != NUM_CONNCLRS; i++) {
		if (connattribsused & (1 << i)) {
			gdImageString(im, gdFontGetTiny(), img_cx - 65,
				img_cy - cpop * 13 + ndrawn * 13 - 9,
				(unsigned char *)connattribstrs[i], colors[CLR_WHITE]);
			gdImageLine(im, img_cx - 65, img_cy - cpop * 13 + ndrawn * 13,
				img_cx - 10, img_cy - cpop * 13 + ndrawn * 13, colors[conncolors[i]]);
			ndrawn++;
		}
	}
}

