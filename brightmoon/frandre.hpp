#ifndef FRANDRE_HPP
#define FRANDRE_HPP

#include <fstream>
#include <list>
#include <cstring>
#include "stdint.h"

#include "arctmp.hpp"
#include "bitreader.hpp"

class FrandreArchive_Base
{
protected:
  struct Entry {
    uint32_t offset;
    uint32_t compsize;
    uint32_t origsize;
    uint32_t checksum;
    char name[0x100];

    inline uint32_t GetOriginalSize() { return origsize; }
    inline uint32_t GetCompressedSize() { return compsize; }
    inline const char * GetEntryName() { return name; }

    bool operator==(const char *rhs) {
      return std::strcmp(name, rhs) == 0;
    }
  };

  typedef std::list<Entry> EntryList;

protected:
  FrandreArchive_Base();
  virtual ~FrandreArchive_Base();

protected:
  bool Open(const char *filename, EntryList &list);
  bool Extract(EntryList::iterator &it, std::ostream &os, bool (*callback)(const char *, void *), void * user);

private:
  uint32_t ReadPackValue(BitReader &);
  void ReadCString(BitReader &, char *string, int max);

private:
  std::ifstream m_file;
};

typedef PBGArchiveTemplate<FrandreArchive_Base> FrandreArchive;

#endif
