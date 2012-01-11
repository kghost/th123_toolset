#ifndef KAGUYA_HPP
#define KAGUYA_HPP

#include <fstream>
#include <list>
#include <cstring>
#include "stdint.h"
#include "arctmp.hpp"

class KaguyaArchive_Base
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
    unsigned char type;
    unsigned char key;
    unsigned char step;
    int block;
    int limit;
  };

protected:
  KaguyaArchive_Base();
  virtual ~KaguyaArchive_Base();

protected:
  bool Open(const char *filename, EntryList &list);
  bool Extract(EntryList::iterator &it, std::ostream &os, bool (*callback)(const char *, void *), void * user);

public:
  void SetArchiveType(int type);

private:
  static const CryptParam m_cryprm1[];
  static const CryptParam m_cryprm2[];

private:
  std::ifstream m_file;
  const CryptParam *m_cryprm;
};

typedef PBGArchiveTemplate<KaguyaArchive_Base> KaguyaArchive;

#endif
