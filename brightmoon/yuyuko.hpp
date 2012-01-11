#ifndef YUYUKO_HPP
#define YUYUKO_HPP

#include <fstream>
#include <string>
#include <list>
#include <cstring>
#include "stdint.h"
#include "arctmp.hpp"

class YuyukoArchive_Base
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

protected:
  YuyukoArchive_Base();
  virtual ~YuyukoArchive_Base();

protected:
  bool Open(const char *filename, EntryList &list);
  bool Extract(EntryList::iterator &entry, std::ostream &os, bool (*callback)(const char *, void *), void * user);

private:
  std::ifstream m_file;
};

typedef PBGArchiveTemplate<YuyukoArchive_Base> YuyukoArchive;

#endif
