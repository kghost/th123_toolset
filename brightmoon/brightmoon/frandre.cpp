#include <fstream>
#include <algorithm>
#include "frandre.hpp"

extern void unlzss(std::istream &in, std::ostream &out);

FrandreArchive_Base::FrandreArchive_Base()
{
}

FrandreArchive_Base::~FrandreArchive_Base()
{
}

bool FrandreArchive_Base::Open(const char *filename, EntryList &list)
{
  m_file.open(filename, std::ios_base::binary);
  if(m_file.fail()) return false;

  std::ifstream::pos_type begpos = m_file.tellg();
  m_file.seekg(0, std::ios_base::end);
  std::ifstream::pos_type endpos = m_file.tellg();
  m_file.seekg(0, std::ios_base::beg);
  std::ifstream::pos_type filesize = endpos - begpos;
  BitReader reader(m_file);
  int signature = reader.Read(32);
  if(signature != 'PBG3') return false;

  uint32_t filecount  = ReadPackValue(reader);
  uint32_t listoffset = ReadPackValue(reader);
  if(listoffset >= filesize) return false;
  if(filecount == 0) return false;

  m_file.seekg(listoffset, std::ios_base::beg);
  reader.Attach(m_file);
  list.clear();
  for(int i = 0; i < filecount; ++i)
  {
    Entry entry;
    ReadPackValue(reader); // ?
    ReadPackValue(reader); // ??
    entry.checksum = ReadPackValue(reader);
    entry.offset   = ReadPackValue(reader);
    entry.origsize = ReadPackValue(reader);
    ReadCString(reader, entry.name, 0x100);
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

bool FrandreArchive_Base::Extract(EntryList::iterator &entry, std::ostream &os, bool (*callback)(const char *, void *), void * user)
{
  if(callback) {
    if(!callback(entry->GetEntryName(), user)) return false;
    if(!callback(" extracting...", user)) return false;
  }
  m_file.seekg(entry->offset, std::ios_base::beg);
  unlzss(m_file, os);
  if(m_file.bad()) {
    if(callback) {
      if(!callback("unexpected error occured.\r\nplease restart.\r\n", user)) return false;
    }
    return false;
  }
  if(callback) {
    if(!callback("finished.\r\n", user)) return false;
  }
  return true;
}

uint32_t FrandreArchive_Base::ReadPackValue(BitReader &reader)
{
  int type = reader.Read(2);
  return reader.Read((type + 1) * 8);
}

void FrandreArchive_Base::ReadCString(BitReader &reader, char *str, int max)
{
  while(max-- && (*str++ = reader.Read(8)));
}
