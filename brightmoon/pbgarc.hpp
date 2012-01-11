#ifndef PBGARC_HPP
#define PBGARC_HPP

#include "stdint.h"

struct PBGArchive
{
  virtual bool Open(const char * filename) = 0;
  virtual bool EnumFirst() = 0;
  virtual bool EnumNext() = 0;
  virtual const char * GetEntryName() = 0;
  virtual uint32_t GetOriginalSize() = 0;
  virtual uint32_t GetCompressedSize() = 0;
  virtual struct PBGArchiveEntry * GetEntry() = 0;
  virtual bool Extract(std::ostream &os, bool (*callback)(const char *, void *), void * user) = 0;
  virtual bool ExtractAll(bool (*callback)(const char *, void *), void * user) = 0;
};

struct PBGArchiveEntry
{
  virtual const char * GetEntryName() = 0;
  virtual uint32_t GetOriginalSize() = 0;
  virtual uint32_t GetCompressedSize() = 0;
  virtual bool Extract(std::ostream &os, bool (*callback)(const char *, void *), void * user) = 0;
};

#endif
