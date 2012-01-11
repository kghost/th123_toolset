#include <sstream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>
#include <new>
#include <vector>
#include <boost/scoped_array.hpp>
#include <Windows.h>
#include "hinanawi.hpp"
#include "mt.hpp"

HinanawiArchive_Base::HinanawiArchive_Base()
{
}

HinanawiArchive_Base::~HinanawiArchive_Base()
{
}

bool HinanawiArchive_Base::Open(const char *filename, EntryList &list)
{
  m_file.open(filename, std::ios_base::binary);
  if(m_file.fail()) return false;

  std::ifstream::pos_type begpos = m_file.tellg();
  m_file.seekg(0, std::ios_base::end);
  std::ifstream::pos_type endpos = m_file.tellg();
  m_file.seekg(0, std::ios_base::beg);
  std::ifstream::pos_type filesize = endpos - begpos;
  if(filesize < 6) return false;

  uint16_t list_count;
  m_file.read(reinterpret_cast<char *>(&list_count), sizeof(list_count));
  if(m_file.eof() || m_file.fail()) return false;
  uint32_t list_size;
  m_file.read(reinterpret_cast<char *>(&list_size), sizeof(list_size));
  if(m_file.eof() || m_file.fail()) return false;
  if(list_count == 0 || list_size == 0) return false;
  if(filesize < 6 + list_size) return false;

  boost::scoped_array<char> list_buf(new (std::nothrow) char[list_size]);
  if(!list_buf) return false;

  m_file.read(list_buf.get(), list_size);
  if(m_file.eof() || m_file.fail()) return false;

  RNG_MT mt(list_size + 6);
  for(uint32_t i = 0; i < list_size; ++i)
    list_buf[i] ^= mt.next_int32() & 0xFF;

  if(!DeserializeList(list_buf.get(), list_count, list_size, filesize, list)) {
    list.clear();
    uint8_t k = 0xC5, t = 0x83;
    for(uint32_t i = 0; i < list_size; ++i) {
      list_buf[i] ^= k; k += t; t +=0x53;
    }
    if(!DeserializeList(list_buf.get(), list_count, list_size, filesize, list))
      return false;
  }
  return true;
}

bool HinanawiArchive_Base::DeserializeList(char *list_buf, uint32_t list_count, uint32_t list_size, uint32_t filesize, EntryList &list)
{
  const char *p = list_buf;
  uint32_t offset = 0;
  for(uint16_t i = 0; i < list_count; ++i) {
    if(offset + 9 > filesize) return false;
    uint8_t name_len = *reinterpret_cast<const uint8_t *>(p + 8);
    if(offset + 9 + name_len > filesize) return false;
    Entry entry = {
      *reinterpret_cast<const uint32_t *>(p),
      *reinterpret_cast<const uint32_t *>(p + 4),
      std::string(p + 9, p + 9 + name_len)
    };
    if(entry.offset < list_size + 6 || entry.offset > filesize) return false;
    if(entry.size > static_cast<uint32_t>(filesize) - entry.offset) return false;
    if(entry.name == "") return false;
	int wlen = MultiByteToWideChar(932, MB_PRECOMPOSED, entry.name.c_str(), entry.name.size(), NULL, 0);
	std::vector<WCHAR> ws(wlen);
	MultiByteToWideChar(932, MB_PRECOMPOSED, entry.name.c_str(), entry.name.size(), &ws[0], ws.size());
	wlen = WideCharToMultiByte(CP_ACP, 0, &ws[0], ws.size(), NULL, 0, NULL, NULL);
	std::vector<char> s(wlen);
	WideCharToMultiByte(CP_ACP, 0, &ws[0], ws.size(), &s[0], s.size(), NULL, NULL);
	entry.name = std::string(s.begin(), s.end());
    list.push_back(entry);
    p += 9 + name_len;
    offset += 9 + name_len;
  }
  return true;
}

bool HinanawiArchive_Base::Extract(EntryList::iterator &entry, std::ostream &os, bool (*callback)(const char *, void *), void * user)
{
  if(callback) {
    if(!callback(entry->GetEntryName(), user)) return false;
    if(!callback(" extracting...", user)) return false;
  }

  boost::scoped_array<char> data(new (std::nothrow) char[entry->size]);
  if(!data) {
    callback(" out of memory\r\n", user);
    return false;
  }
  
  m_file.seekg(entry->offset, std::ios::beg);
  m_file.read(reinterpret_cast<char *>(data.get()), entry->size);
  if(m_file.eof() || m_file.fail()) return false;
  
  uint8_t k = (entry->offset >> 1) | 0x23;
  for(uint32_t i = 0; i < entry->size; ++i)
    data[i] ^= k;

  os.write(data.get(), entry->size);

  if(callback) {
    if(!callback("finished.\r\n", user)) return false;
  }
  return true;
}
