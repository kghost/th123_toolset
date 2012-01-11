#ifndef VIVIT_HPP
#define VIVIT_HPP

#include <fstream>
#include "arctmp.hpp"

/* Vivit archive don't have any filenames... */
class VivitArchive_Base
{
protected:
  struct Entry {
    uint32_t origsize;
    uint32_t compsize;
    uint32_t offset;
    uint32_t resd08; /* what does this mean...? */
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
  VivitArchive_Base();
  virtual ~VivitArchive_Base();

protected:
  bool Open(const char *filename, EntryList &list);
  bool Extract(EntryList::iterator &itearotr, std::ostream &os, bool (*callback)(const char *, void *), void * user);

private:
  std::ifstream m_file;
};

typedef PBGArchiveTemplate<VivitArchive_Base> VivitArchive;

#endif
