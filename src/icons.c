/*-
 * Copyright (c) 2008 Ryan Kwolek
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
 * icons.c - 
 *    Routines to simplify management of icons used within the utility
 */

#include "netgraph.h"
#include "hashtable.h"
#include "icons.h"

LPCHAIN oses[TL_OSES + 1];
int numoses;

////////////////////////////////////////////////////////////////////////////////////////////////


void LoadOSIcons() {
	gdImagePtr img;
	char tmppath[MAX_PATH], *blah;

	#ifdef _WIN32
		WIN32_FIND_DATA ffd;
		HANDLE ffh;
		LPOS lpos;
		
		ffh = FindFirstFile("os_icons\\*", &ffd);
		if (ffh == INVALID_HANDLE_VALUE) {
			printf("Failed to find OS icons!\n");
			return;
		}
		do {
			if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				strcpy(tmppath, "os_icons\\");
				strcat(tmppath, ffd.cFileName);
				img = LoadImageGd(tmppath);
				if (img) {
					lpos = malloc(sizeof(OS));
					
					strncpy(lpos->name, ffd.cFileName, sizeof(lpos->name));
					lpos->name[sizeof(lpos->name) - 1] = 0;
					
					blah = strrchr(lpos->name, '.');
					if (blah)
						*blah = 0;
					lcase(lpos->name);
					
					lpos->icon = img;
					HtInsertValue(lpos->name, lpos, oses, TL_OSES);
					numoses++;
					//gdImageColorTransparent(img, gdImageColorAllocate(img, 0xFF, 0, 0xFF));

				}
			}
		} while (FindNextFile(ffh, &ffd));
		FindClose(ffh);
	#else

	#endif

}


gdImagePtr LoadImageGd(const char *filename) {
	FILE *file;
	gdImagePtr img;
	char blah[4], *tmp;
	long filelen;

	file = fopen(filename, "rb");
	if (!file)
		return NULL;

	if (fread(blah, 1, 4, file) != 4) {
		fclose(file);
		return NULL;
	}
	fseek(file, 0, SEEK_END);
	filelen = ftell(file);
	rewind(file);

	tmp = malloc(filelen);
	fread(tmp, filelen, 1, file);
	fclose(file);

	if (*(uint16_t *)blah == 0xD8FF) {
		img = gdImageCreateFromJpegPtr(filelen, tmp);
	} else if (*(uint32_t *)blah == 'GNP\x89') {
		img = gdImageCreateFromPngPtr(filelen, tmp);
	} else if (*(uint32_t *)blah == '8FIG') {
		img = gdImageCreateFromGifPtr(filelen, tmp);
	} else {
		printf("WARNING: %s is an unhandled image format\n", filename);
		img = NULL;
	}

	free(tmp);
	return img;
}

