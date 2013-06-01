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
 * hashtable.c - 
 *    High performance hashtable routines and general purpose 32 bit hash function
 */


#include "netgraph.h"
#include "hashtable.h"

void HtInsertValue(const char *key, void *newentry, LPCHAIN *table, unsigned int tablelen) {
	unsigned int index = hash((unsigned char *)key) & tablelen;

	if (table[index]) {
		table[index] = (LPCHAIN)realloc(table[index], ((table[index]->numentries + 1) * sizeof(void *)) + 4);
		table[index]->entry[table[index]->numentries] = newentry;
		table[index]->numentries++;
	} else {
		table[index] = (LPCHAIN)malloc(sizeof(CHAIN));
		table[index]->numentries = 1;
		table[index]->entry[0] = newentry;
	}			
}


void HtRemoveValue(const char *key, LPCHAIN *table, unsigned int tablelen) {
	int posreached;
	unsigned int index = hash((unsigned char *)key) & tablelen;

	posreached = 0;
	if (table[index]) {
		int i = 0;
		while (i < table[index]->numentries) {
			if (!strcmp(key, table[index]->entry[i])) {
				table[index]->numentries--;
				free(table[index]->entry[i]);
				posreached = 1;
			}
			if (posreached)
				table[index]->entry[i] = table[index]->entry[i + 1];
			i++;	
		}
	}
}


void *HtGetValue(const char *key, LPCHAIN *table, unsigned int tablelen) {
	unsigned int index = hash((unsigned char *)key) & tablelen;

	if (table[index]) {
		int i;
		for (i = 0; i != table[index]->numentries; i++) {
			if (!strcmp(key, table[index]->entry[i]))
				return table[index]->entry[i];
		}
	}
	return NULL;
}


void HtResetContents(LPCHAIN *table, unsigned int tablelen) {
	unsigned int i, i2;

	for (i = 0; i != tablelen + 1; i++) {
		if (table[i]) {
			for (i2 = 0; i2 != table[i]->numentries; i2++)
				free(table[i]->entry[i2]);
			free(table[i]);
			table[i] = NULL;
		}
	}
}


unsigned int __fastcall hash(unsigned char *key) {
    unsigned int hash = 0;

	while (*key) {
		hash += *key;
		hash += (hash << 10);
		hash ^= (hash >> 6);
		key++;
	}
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

