#include <strstream>
#include <algorithm>
#include <boost/iostreams/copy.hpp>
#include "kanako.hpp"

extern void unlzss(std::istream &in, std::ostream &out);
extern bool thcrypter(std::istream &in, std::ostream &out, int size, unsigned char key, unsigned char step, int block, int limit);

const KanakoArchive_Base::CryptParam
KanakoArchive_Base::m_cryprm[] =
{
  { 0x1b, 0x37, 0x40,  0x2800 },
  { 0x51, 0xe9, 0x40,  0x3000 },
  { 0xc1, 0x51, 0x80,  0x3200 },
  { 0x03, 0x19, 0x400, 0x7800 },
  { 0xab, 0xcd, 0x200, 0x2800 },
  { 0x12, 0x34, 0x80,  0x3200 },
  { 0x35, 0x97, 0x80,  0x2800 },
  { 0x99, 0x37, 0x400, 0x2000 }
};

KanakoArchive_Base::KanakoArchive_Base()
{
}

KanakoArchive_Base::~KanakoArchive_Base()
{
}

bool KanakoArchive_Base::Open(const char *filename, EntryList &list)
{
  m_file.open(filename, std::ios_base::binary);
  if(m_file.fail()) return false;

  std::ifstream::pos_type begpos = m_file.tellg();
  m_file.seekg(0, std::ios_base::end);
  std::ifstream::pos_type endpos = m_file.tellg();
  m_file.seekg(0, std::ios_base::beg);
  std::ifstream::pos_type filesize = endpos - begpos;

  std::strstream headbuf;
  if(!thcrypter(m_file, headbuf, 0x10, 0x1B, 0x37, 0x10, 0x10))
    return false;

  uint32_t magic;
  headbuf.read((char *)&magic, 4);
  if(magic != '1AHT') return false;

  uint32_t listsize;
  uint32_t listcompsize;
  uint32_t filecount;
  headbuf.read((char *)&listsize, 4);
  headbuf.read((char *)&listcompsize, 4);
  headbuf.read((char *)&filecount, 4);
  listsize      -= 123456789UL;
  listcompsize  -= 987654321UL;
  filecount     -= 135792468UL;
  if(listcompsize > filesize) return false;
  
  uint32_t listoffset = static_cast<uint32_t>(filesize) - listcompsize;
  m_file.seekg(listoffset, std::ios_base::beg);

  std::strstream compbuf;
  if(!thcrypter(m_file, compbuf, listcompsize, 0x3e, 0x9b, 0x80, listcompsize))
    return false;
  
  std::strstream listbuf;
  unlzss(compbuf, listbuf);

  for(int i = 0; i < filecount; ++i) {
    Entry entry;
    char buff[4];
    do {
      listbuf.read(buff, 4);
      if(listbuf.fail()) return false;
      for(int j = 0; j < 4 && buff[j]; ++j)
        entry.name.push_back(buff[j]);
    } while(buff[3]);
    listbuf.read((char *)&entry.offset, 4);
    if(listbuf.fail()) return false;
    listbuf.read((char *)&entry.origsize, 4);
    if(listbuf.fail()) return false;
    listbuf.read(buff, 4); // zero padding
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

bool KanakoArchive_Base::Extract(EntryList::iterator &entry, std::ostream &os, bool (*callback)(const char *, void *), void * user)
{
  if(callback) {
    if(!callback(entry->GetEntryName(), user)) return false;
    if(!callback(" extracting...", user)) return false;
  }
  m_file.clear();
  m_file.seekg(entry->offset, std::ios_base::beg);
  int cryidx = GetCryptParamIndex(entry->GetEntryName());
  std::strstream compbuf;
  if(!thcrypter(m_file, compbuf, entry->GetCompressedSize(), 
      m_cryprm[cryidx].key, m_cryprm[cryidx].step, m_cryprm[cryidx].block, m_cryprm[cryidx].limit))
  {
    if(callback) {
      if(!callback("failed to decryption.\r\n", user)) return false;
    }
    return false;
  }
  if(entry->GetCompressedSize() == entry->GetOriginalSize()) {
    boost::iostreams::copy(compbuf, os);
  } else {
    unlzss(compbuf, os);
  }
  if(callback) {
    if(!callback("finished.\r\n", user)) return false;
  }
  return true;
}

int KanakoArchive_Base::GetCryptParamIndex(const char *entryname)
{
  char index = 0;
  while(*entryname) index += *entryname++;
  return index & 7;
}
