#ifndef SUICA_HPP
#define SUICA_HPP

#include <fstream>
#include <string>
#include <list>
#include <cstring>
#include "stdint.h"
#include "arctmp.hpp"

class SuicaArchive_Base
{
protected:
  struct Entry {
    uint32_t offset;
    uint32_t size;
    std::string name;

    inline uint32_t GetOriginalSize() { return size; }
    inline uint32_t GetCompressedSize() { return size; }
    inline const char * GetEntryName() { return name.c_str(); }

    bool operator==(const char *rhs) {
      return std::strcmp(name.c_str(), rhs) == 0;
    }
  };

  typedef std::list<Entry> EntryList;

protected:
  SuicaArchive_Base();
  virtual ~SuicaArchive_Base();

protected:
  bool Open(const char *filename, EntryList &list);
  bool Extract(EntryList::iterator &entry, std::ostream &os, bool (*callback)(const char *, void *), void * user);

private:
  std::ifstream m_file;
};

typedef PBGArchiveTemplate<SuicaArchive_Base> SuicaArchive;

#endif
