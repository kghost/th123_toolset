// darkmoon.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Windows.h>
#include <new>
#include <list>
#include <string>
#include <boost/scoped_array.hpp>
#include "mt.hpp"

struct ArchiveFile {
	std::string name;
	unsigned __int32 size;
	unsigned __int32 offset;
};

void find_file (std::list<ArchiveFile> &l,std::string path)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile ((path + "*.*").c_str(), &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		printf ("FindFirstFile failed (%d) on directory %s\n", GetLastError(), path.c_str());
		return;
	}
	do {
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (strcmp(FindFileData.cFileName, ".") == 0 || strcmp(FindFileData.cFileName, "..") == 0)
				continue;

			find_file(l, path + FindFileData.cFileName + '/');
			continue;
		}

		if (FindFileData.nFileSizeHigh) {
			printf (TEXT("File %s too big\n"), FindFileData.cFileName);
			continue;
		}

		ArchiveFile a = { path + FindFileData.cFileName, FindFileData.nFileSizeLow };
		l.push_back(a);
		printf (TEXT("File %s length %d\n"), a.name.c_str(), a.size);
	} while (FindNextFile(hFind, &FindFileData));
	FindClose(hFind);
}

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc <= 1) {
		printf ("Usage : %s directory\n", argv[0]);
		return -1;
	}

	int i = GetCurrentDirectory(0, NULL);

	boost::scoped_array<TCHAR> dir(new (std::nothrow) TCHAR[i + 32]);
	if(!dir) return -1;

	GetCurrentDirectory(i, dir.get());

	strcat(dir.get(), TEXT("/archive.dat"));

	if (!SetCurrentDirectory(argv[1])) {
		printf ("SetCurrentDirectory error : %d\n", GetLastError());
		return -1;
	}

	// list files
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile (TEXT("*.*"), &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
		printf ("FindFirstFile failed (%d)\n", GetLastError());
		return -1;
	}

	std::list<ArchiveFile> l;
	do {
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (strcmp(FindFileData.cFileName, ".") == 0 || strcmp(FindFileData.cFileName, "..") == 0)
				continue;

			find_file(l, std::string(FindFileData.cFileName) + '/');
			continue;
		}

		if (FindFileData.nFileSizeHigh) {
			printf (TEXT("File %s too big\n"), FindFileData.cFileName);
			continue;
		}

		if (strcmp(FindFileData.cFileName, "archive.dat") == 0)
			continue;

		ArchiveFile a = { std::string(FindFileData.cFileName), FindFileData.nFileSizeLow };
		l.push_back(a);
		printf (TEXT("File %s length %d\n"), a.name.c_str(), a.size);
	} while (FindNextFile(hFind, &FindFileData));
	FindClose(hFind);

	// calculate header size
	unsigned __int32 list_size = 0;
	for (std::list<ArchiveFile>::iterator i = l.begin(); i != l.end(); ++i) {
		list_size += 9 + i->name.length();
	}

	// build header
	boost::scoped_array<char> list_buf(new (std::nothrow) char[list_size]);
	if(!list_buf) return -1;

	char* p = list_buf.get();
	int offset = 6 + list_size;
	for (std::list<ArchiveFile>::iterator i = l.begin(); i != l.end(); ++i) {
		*reinterpret_cast<unsigned __int32 *>(p) = offset;
		*reinterpret_cast<unsigned __int32 *>(p + 4) = i->size;
		i->offset = offset;
		offset += i->size;
		int namelen = i->name.length();
		*reinterpret_cast<unsigned __int8 *>(p + 8) = namelen;
		memcpy(p+9, i->name.c_str(), namelen);
		p += 9 + namelen;
	}

	// encrypt header
	RNG_MT mt(list_size + 6);
	for(unsigned __int32 i = 0; i < list_size; ++i)
		list_buf[i] ^= mt.next_int32() & 0xFF;

	unsigned __int8 k = 0xC5, t = 0x83;
	for(unsigned __int32 i = 0; i < list_size; ++i) {
		list_buf[i] ^= k; k += t; t +=0x53;
	}

	// write header
	printf ("Create Archive %s\n", dir.get());
	HANDLE hArchive = CreateFile (dir.get(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (hArchive == INVALID_HANDLE_VALUE) {
		printf ("CreateFile failed (%d)\n", GetLastError());
		return -1;
	}
	unsigned __int16 list_count = l.size();
	DWORD tmp;
	if (WriteFile (hArchive, &list_count, 2, &tmp, NULL) == 0) {
		printf ("WriteFile failed (%d)\n", GetLastError());
	}
	if (WriteFile (hArchive, &list_size, 4, &tmp, NULL) == 0) {
		printf ("WriteFile failed (%d)\n", GetLastError());
	}
	if (WriteFile (hArchive, list_buf.get(), list_size, &tmp, NULL) == 0) {
		printf ("WriteFile failed (%d)\n", GetLastError());
	}

	// write files
	for (std::list<ArchiveFile>::iterator i = l.begin(); i != l.end(); ++i) {
		HANDLE hFile = CreateFile (i->name.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
		if (hFile == INVALID_HANDLE_VALUE) {
			printf ("OpenFile failed (%d)\n", GetLastError());
		}

		boost::scoped_array<char> file_buf(new (std::nothrow) char[i->size]);
		if(!file_buf) return -1;

		if (ReadFile (hFile, file_buf.get(), i->size, &tmp, NULL) == 0) {
			printf ("WriteFile failed (%d)\n", GetLastError());
		}

		unsigned __int8 k = (i->offset >> 1) | 0x23;
		for(unsigned __int32 j = 0; j < i->size; ++j)
			file_buf[j] ^= k;

		if (WriteFile (hArchive, file_buf.get(), i->size, &tmp, NULL) == 0) {
			printf ("WriteFile failed (%d)\n", GetLastError());
		}
	}

	return 0;
}

