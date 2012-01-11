#include <sstream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>
#include <new>
#include <boost/scoped_array.hpp>
#include "suica.hpp"

SuicaArchive_Base::SuicaArchive_Base()
{
}

SuicaArchive_Base::~SuicaArchive_Base()
{
}

bool SuicaArchive_Base::Open(const char *filename, EntryList &list)
{
  m_file.open(filename, std::ios_base::binary);
  if(m_file.fail()) return false;

  std::ifstream::pos_type begpos = m_file.tellg();
  m_file.seekg(0, std::ios_base::end);
  std::ifstream::pos_type endpos = m_file.tellg();
  m_file.seekg(0, std::ios_base::beg);
  std::ifstream::pos_type filesize = endpos - begpos;
  if(filesize < 2) return false;

  uint16_t list_count;
  m_file.read(reinterpret_cast<char *>(&list_count), sizeof(list_count));
  if(m_file.eof() || m_file.fail()) return false;

  uint32_t list_size = list_count * 0x6C;
  if(filesize < list_size + 2) return false;

  boost::scoped_array<char> list_buf(new (std::nothrow) char[list_size]);
  if(!list_buf) return false;

  m_file.read(list_buf.get(), list_size);
  if(m_file.eof() || m_file.fail()) return false;

  uint8_t k = 0x64, t = 0x64;
  for(uint32_t i = 0; i < list_size; ++i) {
    list_buf[i] ^= k;
    k += t; t += 0x4D;
  }

  const char *p = list_buf.get();
  for(uint16_t i = 0; i < list_count; ++i) {
    Entry entry = {
      *reinterpret_cast<const uint32_t *>(p + 0x68),
      *reinterpret_cast<const uint32_t *>(p + 0x64),
      std::string(p, p + 0x64)
    };
    if(entry.offset < list_size + 2 || entry.offset > filesize) return false;
    if(entry.size > static_cast<uint32_t>(filesize) - entry.offset) return false;
    if(entry.name == "") return false;
    list.push_back(entry);
    p += 0x6c;
  }
  return true;
}

bool SuicaArchive_Base::Extract(EntryList::iterator &entry, std::ostream &os, bool (*callback)(const char *, void *), void * user)
{
  if(callback) {
    if(!callback(entry->GetEntryName(), user)) return false;
    if(!callback(" extracting...", user)) return false;
  }
  m_file.seekg(entry->offset, std::ios::beg);

  // boost.iostreams.copyのcount指定バージョンはないのかね？
  char buff[1024];
  uint32_t left = entry->size;
  while(left > sizeof buff) {
    m_file.read(buff, sizeof buff);
    if(m_file.eof() || m_file.fail()) return false;
    os.write(buff, sizeof buff);
    left -= sizeof buff;
  }
  if(left > 0) {
    m_file.read(buff, left);
    if(m_file.eof() || m_file.fail()) return false;
    os.write(buff, left);
  }

  if(callback) {
    if(!callback("finished.\r\n", user)) return false;
  }
  return false;
}
