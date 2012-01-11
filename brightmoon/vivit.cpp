#include <sstream>
#include <iomanip>
#include <algorithm>
#include "vivit.hpp"

extern void unlzss(std::istream &in, std::ostream &out);

VivitArchive_Base::VivitArchive_Base()
{
}

VivitArchive_Base::~VivitArchive_Base()
{
}

bool VivitArchive_Base::Open(const char *filename, EntryList &list)
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
  if(magic != '\x1AGBP') return false;

  m_file.seekg(8, std::ios_base::beg);
  if(m_file.fail()) return false;

  uint32_t count;
  m_file.read((char *)&count, 4);
  if(m_file.eof()) return false;

  for(uint32_t i = 0; i < count; ++i) {
    Entry entry;
    struct {
      uint32_t size;
      uint32_t offset;
      uint32_t resd08;
    } temp;
    m_file.read((char *)&temp, sizeof(temp));
    if(m_file.eof()) return false;
    std::stringstream ss;
    ss << "DATA" << std::setfill('0') << std::setw(4) << i;
    entry.origsize = temp.size;
    entry.offset = temp.offset;
    entry.resd08 = temp.resd08;
    entry.name = ss.str();
    list.push_back(entry);
  }
  EntryList::iterator it   = list.begin();
  EntryList::iterator next = list.begin();
  for(++next; next != list.end(); ++it, ++next)
  {
    it->compsize = next->offset - it->offset;
  }
  it->compsize = static_cast<uint32_t>(filesize) - it->offset;
  return true;
}


bool VivitArchive_Base::Extract(EntryList::iterator &entry, std::ostream &os, bool (*callback)(const char *, void *), void * user)
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
