#ifndef YUMEMI_HPP
#define YUMEMI_HPP

#include <fstream>
#include <list>
#include <cstring>
#include "stdint.h"
#include "arctmp.hpp"

class YumemiArchive_Base
{
protected:
  struct Entry {
    uint32_t offset;
    uint16_t compsize;
    uint16_t origsize;
    uint8_t key;
    char name[13];

    inline uint32_t GetOriginalSize() { return origsize; }
    inline uint32_t GetCompressedSize() { return compsize; }
    inline const char * GetEntryName() { return name; }

    bool operator==(const char *rhs) {
      return std::strcmp(name, rhs) == 0;
    }
  };

  typedef std::list<Entry> EntryList;

protected:
  YumemiArchive_Base();
  virtual ~YumemiArchive_Base();

protected:
  bool Open(const char *filename, EntryList &list);
  bool Extract(EntryList::iterator &it, std::ostream &os, bool (*callback)(const char *, void *), void * user);

private:
  bool DeserializeList(std::istream &is, uint32_t list_count, uint32_t filesize, EntryList &list);
  static bool ValidateName(const char *name);

private:
  std::ifstream m_file;
};

typedef PBGArchiveTemplate<YumemiArchive_Base> YumemiArchive;

#endif
