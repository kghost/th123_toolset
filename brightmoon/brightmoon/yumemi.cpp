#include <algorithm>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include "yumemi.hpp"
#include "xor.hpp"
#include "limit.hpp"
#include "yumemicrypt.hpp"

extern bool unerle(std::istream &in, std::ostream &out);

#define isfchr(c) ((c)>=' '&&(c)!='+'&&(c)!=','&&(c)!=';'&&(c)!='='&&(c)!='['&&(c)!=']'&&(c)!='.')

YumemiArchive_Base::YumemiArchive_Base()
{
}

YumemiArchive_Base::~YumemiArchive_Base()
{
}

bool YumemiArchive_Base::Open(const char *filename, EntryList &list)
{
  m_file.open(filename, std::ios_base::binary);
  if(m_file.fail()) return false;

  std::ifstream::pos_type begpos = m_file.tellg();
  m_file.seekg(0, std::ios_base::end);
  std::ifstream::pos_type endpos = m_file.tellg();
  m_file.seekg(0, std::ios_base::beg);
  std::ifstream::pos_type filesize = endpos - begpos;

  uint16_t entrysize;
  uint16_t entrynum;
  uint8_t  entrykey;
  char     padding[9];
  m_file.read((char *)&entrysize, 2);
  if(m_file.fail()) return false;
  m_file.read(padding, 2);
  if(m_file.fail()) return false;
  m_file.read((char *)&entrynum, 2);
  if(m_file.fail()) return false;
  m_file.read((char *)&entrykey, 1);
  if(m_file.fail()) return false;
  m_file.read(padding, 9);
  if(m_file.fail()) return false;

  if(entrysize > filesize) return false;
  if(entrysize & 0x1F || entrysize / 0x20 < entrynum)
    return false;

  boost::iostreams::filtering_istream in;
  in.push(yumemicrypt_input_filter(entrykey));
  in.push(limit_input_filter(entrysize));
  in.push(m_file);
  return DeserializeList(in, entrynum, filesize, list);
}

bool YumemiArchive_Base::DeserializeList(std::istream &is, uint32_t list_count, uint32_t filesize, EntryList &list)
{
  for(uint16_t i = 0; i < list_count; ++i) {
    Entry entry;
    uint16_t magic;
    char     padding[8];
    is.read((char *)&magic, 2);
    if(is.fail()) return false;
    is.read((char *)&entry.key, 1);
    if(is.fail()) return false;
    is.read(entry.name, 13);
    if(is.fail()) return false;
    is.read((char *)&entry.compsize, 2);
    if(is.fail()) return false;
    is.read((char *)&entry.origsize, 2);
    if(is.fail()) return false;
    is.read((char *)&entry.offset, 4);
    if(is.fail()) return false;
    is.read(padding, 8);
    if(is.fail()) return false;

    if(magic == 0) break;

    if(magic != 0x9595 && magic != 0xF388) return false;
    if(entry.offset >= filesize) return false;
    if((uint32_t)filesize - entry.offset < entry.compsize) return false;
    if(!ValidateName(entry.name)) return false;
    list.push_back(entry);
  }
  return true;
}

bool YumemiArchive_Base::Extract(EntryList::iterator &entry, std::ostream &os, bool (*callback)(const char *, void *), void * user)
{
  if(callback) {
    if(!callback(entry->GetEntryName(), user)) return false;
    if(!callback(" extracting...", user)) return false;
  }
  m_file.clear();
  m_file.seekg(entry->offset, std::ios_base::beg);
  if(m_file.fail()) return false;

  namespace io = boost::iostreams;
  io::filtering_istream in;
  in.push(xor_input_filter(entry->key));
  in.push(limit_input_filter(entry->compsize));
  in.push(m_file);
  if(entry->GetCompressedSize() == entry->GetOriginalSize()) {
    boost::iostreams::copy(in, os);
  } else {
    unerle(in, os);
  }
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

bool YumemiArchive_Base::ValidateName(const char *name)
{
  int i, j;
  /* ファイル名は8.3形式でないとダメ */
  for(i = j = 0; i < 8 && isfchr(name[i]); ++i);
  if(name[i] == '.')
    for(j = 1; j < 4 && isfchr(name[i + j]); ++j);
  return (name[i + j] == '\0') && i && (j != 1);
}
