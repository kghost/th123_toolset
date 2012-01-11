#include <strstream>
#include <algorithm>
#include "kaguya.hpp"

#define ARRAYSIZE(x) (sizeof(x) / sizeof(x)[0])

extern void unlzss(std::istream &in, std::ostream &out);
extern bool thcrypter(std::istream &in, std::ostream &out, int size, unsigned char key, unsigned char step, int block, int limit);

const KaguyaArchive_Base::CryptParam
KaguyaArchive_Base::m_cryprm1[] =
{
  { 0x4d, 0x1b, 0x37, 0x40, 0x2000 },
  { 0x54, 0x51, 0xe9, 0x40, 0x3000 },
  { 0x41, 0xc1, 0x51, 0x1400, 0x2000 },
  { 0x4a, 0x03, 0x19, 0x1400, 0x7800 },
  { 0x45, 0xab, 0xcd, 0x200, 0x1000 },
  { 0x57, 0x12, 0x34, 0x400, 0x2800 },
  { 0x2d, 0x35, 0x97, 0x80, 0x2800 },
  { 0x2a, 0x99, 0x37, 0x400, 0x1000 }
};

const KaguyaArchive_Base::CryptParam
KaguyaArchive_Base::m_cryprm2[] =
{
  { 0x4d, 0x1b, 0x37, 0x40,  0x2800 },
  { 0x54, 0x51, 0xe9, 0x40,  0x3000 },
  { 0x41, 0xc1, 0x51, 0x400, 0x400 },
  { 0x4a, 0x03, 0x19, 0x400, 0x400 },
  { 0x45, 0xab, 0xcd, 0x200, 0x1000 },
  { 0x57, 0x12, 0x34, 0x400, 0x400 },
  { 0x2d, 0x35, 0x97, 0x80,  0x2800 },
  { 0x2a, 0x99, 0x37, 0x400, 0x1000 }
};

KaguyaArchive_Base::KaguyaArchive_Base()
  : m_cryprm(m_cryprm1)
{
}

KaguyaArchive_Base::~KaguyaArchive_Base()
{
}

bool KaguyaArchive_Base::Open(const char *filename, EntryList &list)
{
  m_file.open(filename, std::ios_base::binary);
  if(m_file.fail()) return false;

  std::ifstream::pos_type begpos = m_file.tellg();
  m_file.seekg(0, std::ios_base::end);
  std::ifstream::pos_type endpos = m_file.tellg();
  m_file.seekg(0, std::ios_base::beg);
  std::ifstream::pos_type filesize = endpos - begpos;

  uint32_t magic;
  m_file.read((char *)&magic, 4);
  if(magic != 'ZGBP') return false;

  std::strstream headbuf;
  if(!thcrypter(m_file, headbuf, 12, 0x1b, 0x37, 0x0c, 0x400))
    return false;
  
  uint32_t filecount;
  uint32_t listoffset;
  uint32_t listsize;
  headbuf.read((char *)&filecount, 4);
  headbuf.read((char *)&listoffset, 4);
  headbuf.read((char *)&listsize, 4);
  filecount   -= 123456;
  listoffset  -= 345678;
  listsize    -= 567891;
  if(listoffset >= filesize) return false;
 
  std::strstream compbuf;
  m_file.seekg(listoffset, std::ios_base::beg);
  if(!thcrypter(m_file, compbuf, static_cast<uint32_t>(filesize) - listoffset,
        62, 155, 0x80, 0x400))
    return false;

  std::strstream listbuf;
  unlzss(compbuf, listbuf);

  for(int i = 0; i < filecount; ++i) {
    Entry entry;
    for(;;) {
      char c;
      listbuf.read(&c, 1);
      if(listbuf.fail()) return false;
      if(!c) break;
      entry.name.push_back(c);
    }
    listbuf.read((char *)&entry.offset, 4);
    if(listbuf.fail()) return false;
    listbuf.read((char *)&entry.origsize, 4);
    if(listbuf.fail()) return false;
    entry.origsize -= 4;
    if(entry.offset >= filesize) return false;
    char dummy[4];
    listbuf.read(dummy, 4);
    if(listbuf.fail()) return false;
    list.push_back(entry);
  }
  EntryList::iterator it   = list.begin();
  EntryList::iterator next = list.begin();
  for(++next; next != list.end(); ++it, ++next)
  {
    it->compsize = next->offset - it->offset;
  }
  it->compsize = listoffset - it->offset;
  return true;
}

bool KaguyaArchive_Base::Extract(EntryList::iterator &entry, std::ostream &os, bool (*callback)(const char *, void *), void *user)
{
  if(callback) {
    if(!callback(entry->GetEntryName(), user)) return false;
    if(!callback(" extracting...", user)) return false;
  }
  m_file.clear();
  m_file.seekg(entry->offset, std::ios_base::beg);
  std::strstream crypbuf;
  unlzss(m_file, crypbuf);
  if(m_file.bad()) {
    if(callback) {
      if(!callback("unexpected error occured!\r\nplease restart.\r\n", user)) return false;
    }
    return false;
  }
  char magic[4];
  crypbuf.read(magic, 4);
  int prmidx;
  for(prmidx = 0; prmidx < 8; ++prmidx)
    if(m_cryprm[prmidx].type == magic[3]) break;
  if(memcmp(magic, "edz", 3) != 0 || prmidx == 8) {
    if(callback) {
      if(!callback("data may be corrupted!\r\n", user)) return false;
    }
    return false;
  }
  if(!thcrypter(crypbuf, os, entry->origsize,
        m_cryprm[prmidx].key, m_cryprm[prmidx].step, m_cryprm[prmidx].block, m_cryprm[prmidx].limit)) {
    if(callback) {
      if(!callback("failed to decryption.\r\n", user)) return false;
    }
    return false;
  }
  if(callback) {
    if(!callback("finished.\r\n", user)) return false;
  }
  return true;
}

void KaguyaArchive_Base::SetArchiveType(int type)
{
  if(type == 0) {
    m_cryprm = m_cryprm1;
  } else {
    m_cryprm = m_cryprm2;
  }
}
