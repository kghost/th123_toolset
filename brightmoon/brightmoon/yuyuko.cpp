#include <strstream>
#include <algorithm>
#include "yuyuko.hpp"

extern void unlzss(std::istream &in, std::ostream &out);

YuyukoArchive_Base::YuyukoArchive_Base()
{
}

YuyukoArchive_Base::~YuyukoArchive_Base()
{
}

bool YuyukoArchive_Base::Open(const char *filename, EntryList &list)
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
  if(m_file.eof()) return false;
  if(magic != '4GBP') return false;

  uint32_t filecount;
  uint32_t listoffset;
  uint32_t listsize;
  m_file.read((char *)&filecount, 4);
  if(m_file.eof()) return false;
  m_file.read((char *)&listoffset, 4);
  if(m_file.eof()) return false;
  m_file.read((char *)&listsize, 4);
  if(m_file.eof()) return false;
  if(listoffset >= filesize) return false;

  m_file.seekg(listoffset, std::ios_base::beg);
  
  std::strstream listbuf;
  unlzss(m_file, listbuf);
  if(m_file.bad()) return false;

  std::streamsize listbufsize = listbuf.pcount();
  if(listsize > listbufsize) return false;
  
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
    if(entry.offset >= filesize) return false;
    char dummy[4];
    listbuf.read(dummy, 4); // zero padding
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

bool YuyukoArchive_Base::Extract(EntryList::iterator &entry, std::ostream &os, bool (*callback)(const char *, void *), void * user)
{
  if(callback) {
    if(!callback(entry->GetEntryName(), user)) return false;
    if(!callback(" extracting...", user)) return false;
  }
  m_file.clear();
  m_file.seekg(entry->offset, std::ios_base::beg);
  if(m_file.fail()) return false;
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
