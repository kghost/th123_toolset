// istring.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <stdlib.h>   // For _MAX_PATH definition
#include <stdio.h>
#include <malloc.h>

int mystrlen (char * s)
{
	int i = 0;
	while (s[i] && !(s[i] == '\r' && s[i+1] == '\n')) i++;
	return i;
}

int _tmain(int argc, _TCHAR* argv[])
{
	FILE * fmod = fopen ("th123.exe", "rb+");
	if (fmod == NULL) {
		perror ("fopen");
		return -1;
	}

	FILE * fin = fopen ("strings.txt", "rb");
	if (fin == NULL) {
		perror ("fopen");
		return -1;
	}

	fseek (fin, 0, SEEK_END);
	long sz = ftell(fin);
	char * buf = (char*)malloc(sz + 1);
	fseek (fin, 0, SEEK_SET);
	fread(buf, sz, 1, fin);
	buf[sz] = 0;
	fclose (fin);

	int floc = 0;
	while (floc < sz) {
		int offset = 0, length = 0;
		for (int i = 0; i < 8; i++) {
			offset *= 16;
			if (buf[floc + i] >= 'a' && buf[floc + i] <= 'f')
				offset += buf[floc + i] - 'a' + 10;
			else if (buf[floc + i] >= '0' && buf[floc + i] <= '9')
				offset += buf[floc + i] - '0';
		}

		for (int i = 0; i < 8; i++) {
			length *= 16;
			if (buf[floc + i + 9] >= 'a' && buf[floc + i + 9] <= 'f') 
				length += buf[floc + i + 9] - 'a' + 10;
			else if (buf[floc + i + 9] >= '0' && buf[floc + i + 9] <= '9')
				length += buf[floc + i + 9] - '0';
		}

		floc += 18;

		int len = mystrlen (buf + floc);

		char * str = (char*)malloc(len + 1);
		memcpy (str, buf + floc, len);
		str[len] = 0;
		floc += len;
		while (buf[floc] == '\r' || buf[floc] == '\n') floc++;

		int sz = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str, -1, NULL, 0);
		WCHAR *o = (WCHAR*)malloc ((sz + 1)*sizeof(WCHAR));
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str, -1, o, sz);
		o[sz] = 0;
		free(str);

		str = (char*)malloc (length);
		memset(str, 0, length);
		WideCharToMultiByte(936, 0, o, -1, str, length, NULL, NULL);
		fseek(fmod, offset, SEEK_SET);
		if (fwrite(str, length, 1, fmod) < 0)
			perror ("fwrite");
		free(str);
		free(o);
	}

	fclose (fmod);

	return 0;
}
