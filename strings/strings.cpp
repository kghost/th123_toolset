// strings.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <stdlib.h>   // For _MAX_PATH definition
#include <stdio.h>
#include <malloc.h>

struct range {
	int offset;
	int length;
};

struct range sector[] = {
	{ 0x446000, 0x49200 },
	{ 0, 0 },
};

void find (FILE * fout, char *string, int offset, int length)
{
	int start = 0, end = 0, rank = 0, first_byte = 0;
	unsigned char * s = (unsigned char *)string;
	for (int i = 0; i < length; i++) {
		if (first_byte) {
			if (s[i] >= 0x40 && s[i] <= 0xfc && s[i] != 0x7f) {
				if (!start) start = i - 1;
				rank++;
				first_byte = 0;
				continue;
			} else {
				start = 0, end = 0, rank = 0, first_byte = 0;
			}
		}

		if (s[i] == 0) {
			if (rank > 1) {
				char *t = (char*)malloc (i - start + 1);
				memcpy (t, string + start, i - start);
				t[i - start] = 0;

				int sz = MultiByteToWideChar(932, MB_PRECOMPOSED, t, -1, NULL, 0);
				WCHAR *o = (WCHAR*)malloc ((sz + 1)*sizeof(WCHAR));
				MultiByteToWideChar(932, MB_PRECOMPOSED, t, -1, o, sz);
				o[sz] = 0;
				free(t);

				sz = WideCharToMultiByte(CP_ACP, 0, o, -1, NULL, 0, NULL, NULL);
				t = (char*)malloc (sz + 1);
				WideCharToMultiByte(CP_ACP, 0, o, -1, t, sz, NULL, NULL);
				t[sz] = 0;
				fprintf (fout, "%8x %8x %s\r\n", offset + start, i - start, t);
				free(t);
				free(o);
			}
			start = 0, end = 0, rank = 0, first_byte = 0;
		} else if (s[i] <= 0x1f || s[i] == 0x7f || s[i] == 0x80 || s[i] == 0xa0 || (s[i] >= 0xf0 && s[i] <= 0xff)) {
			start = 0, end = 0, rank = 0, first_byte = 0;
		} else if ((s[i] >= 0x81 && s[i] <= 0x9f) || (s[i] >= 0xe0 && s[i] <= 0xef)) {
			first_byte = s[i];
		} else if (s[i] >= 0xa1 && s[i] <= 0xdf) {
			start = 0, end = 0, rank = 0, first_byte = 0;
			//if (!start) start = i;
			//rank++;
		} else {
		}
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	FILE * fin = fopen ("th123.exe", "rb");
	if (fin == NULL) {
		perror ("fopen");
		return -1;
	}

	FILE * fout = fopen ("strings.txt", "wb+");
	if (fout == NULL) {
		perror ("fopen");
		return -1;
	}

	for (int i = 0; sector[i].offset && sector[i].length; i++) {
		char * s = (char*)malloc (sector[i].length);
		fseek(fin, sector[i].offset, SEEK_SET);
		fread(s, sector[i].length, 1, fin);
		find(fout, s, sector[i].offset, sector[i].length);
		free(s);
	}

	fclose (fout);
	fclose (fin);

	return 0;
}
