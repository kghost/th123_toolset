#ifndef BITREADER_HPP
#define BITREADER_HPP

#ifndef min
#define min(x, y) ((x) < (y) ? (x) : (y))
#endif

class BitReader
{
private:
  unsigned char m_cbuf;
  unsigned char m_mask;
  std::istream *m_stream;
  
public:
  BitReader(std::istream &stream) :
    m_stream(&stream), m_cbuf(0), m_mask(0x80) {}

  void Attach(std::istream &stream)
  {
    this->m_stream = &stream;
    this->m_cbuf = 0;
    this->m_mask = 0x80;
  }

  int Read(int nBits)
  {
    int nResult = 0;
    for(unsigned int nDestBit = 1 << (nBits - 1); nDestBit; nDestBit >>= 1) {
      if(this->m_mask == 0x80) {
        m_stream->read((char *)&this->m_cbuf, 1);
        if(m_stream->eof()) this->m_cbuf = 0;
      }
      if(this->m_cbuf & this->m_mask) nResult |= nDestBit;
      this->m_mask >>= 1;
      if(!this->m_mask) this->m_mask = 0x80;
    }
    return nResult;
  }
  
  int ReadL(int nBits) {
    int nResult = 0;
    for(int i = 0; nBits > 0; ++i, nBits -= 8) {
      nResult |= Read(min(8, nBits)) << (i * 8);
    }
    return nResult;
  }
};

#endif
