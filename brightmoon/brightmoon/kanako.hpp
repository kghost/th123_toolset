#ifndef KANAKO_HPP
#define KANAKO_HPP

#include <fstream>
#include <list>
#include <cstring>
#include "stdint.h"
#include "arctmp.hpp"

class KanakoArchive_Base
{
protected:
  struct Entry {
    uint32_t offset;
    uint32_t compsize;
    uint32_t origsize;
    std::string name;

    inline uint32_t GetOriginalSize() { return origsize; }
    inline uint32_t GetCompressedSize() { return compsize; }
    inline const char * GetEntryName() { return name.c_str(); }

    bool operator==(const char *rhs) {
      return std::strcmp(name.c_str(), rhs) == 0;
    }
  };

  typedef std::list<Entry> EntryList;

  struct CryptParam {
    unsigned char key;
    unsigned char step;
    int block;
    int limit;
  };

protected:
  KanakoArchive_Base();
  virtual ~KanakoArchive_Base();

protected:
  bool Open(const char *filename, EntryList &list);
  bool Extract(EntryList::iterator &itearotr, std::ostream &os, bool (*callback)(const char *, void *), void * user);

private:
  int GetCryptParamIndex(const char *entryname);

private:
  static const CryptParam m_cryprm[];

private:
  std::ifstream m_file;
};

typedef PBGArchiveTemplate<KanakoArchive_Base> KanakoArchive;

#endif
