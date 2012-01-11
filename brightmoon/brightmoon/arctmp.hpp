#ifndef ARCTMP_HPP
#define ARCTMP_HPP

#include <list>
#include "pbgarc.hpp"

template <class Impl>
class PBGArchiveTemplate : public PBGArchive, public Impl
{
  //friend class PBGArchiveTemplate<Impl>::Entry;
private:
  typedef std::list<typename Impl::Entry> EntryList;
  typedef typename EntryList::iterator EntryIterator;

public:
  class Entry : public PBGArchiveEntry
  {
    friend class PBGArchiveTemplate<Impl>;
  private:
    typedef PBGArchiveTemplate<Impl> _Parent;
    typedef typename _Parent::EntryIterator _EntryIterator;

  private:
    _EntryIterator it;
    _Parent *parent;

  private:
    Entry(_Parent *p, _EntryIterator &vi) {
      parent = p;
      it = vi;
    }

  public:
    Entry() : parent(NULL), it() {}
    Entry(const Entry &orig) {
      parent = orig.parent;
      it = orig.it;
    }

  public:
    uint32_t GetOriginalSize() {
      return it->GetOriginalSize();
    }

    uint32_t GetCompressedSize() {
      return it->GetCompressedSize();
    }

    const char * GetEntryName() {
      return it->GetEntryName();
    }

    bool Extract(std::ostream &os, bool (*callback)(const char *, void *), void * user) {
      return parent->Extract(it, os, callback, user);
    }
  };

public:
  PBGArchiveTemplate() : Impl() {}
  virtual ~PBGArchiveTemplate() {}

public:
  virtual bool Open(const char * filename) {
    return Impl::Open(filename, m_entries);
  }

  virtual bool EnumFirst() {
    m_entry = m_entries.begin();
    return m_entry != m_entries.end();
  }

  virtual bool EnumNext() {
    return m_entry != m_entries.end() &&
         ++m_entry != m_entries.end();
  }
  virtual const char * GetEntryName() {
    return m_entry->GetEntryName();
  }

  virtual uint32_t GetOriginalSize() {
    return m_entry->GetOriginalSize();
  }

  virtual uint32_t GetCompressedSize() {
    return m_entry->GetCompressedSize();
  }

  virtual struct PBGArchiveEntry * GetEntry() {
    return new Entry(this, m_entry);
  }

  virtual bool Extract(std::ostream &os, bool (*callback)(const char *, void *), void * user) {
    return Extract(m_entry, os, callback, user);
  }

  virtual bool ExtractAll(bool (*callback)(const char *, void *), void * user) {
    return false;
  }

private:
  bool Extract(EntryIterator &entry, std::ostream &os, bool (*callback)(const char *, void *), void * user) {
    return Impl::Extract(entry, os, callback, user);
  }

private:
  EntryList m_entries;
  EntryIterator m_entry;
};

#endif
