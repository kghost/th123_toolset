#include <iostream>
#include "BitReader.hpp"

#define DICT_SIZE 0x2000
void unlzss(std::istream &in, std::ostream &out)
{
  unsigned char dict[DICT_SIZE];
  int dictop = 1;

  memset(dict, 0, DICT_SIZE);
  BitReader reader(in);
  for(;;) {
    int flag = reader.Read(1);
    if(flag) {
      int c = reader.Read(8);
      out.write((char *)&c, 1);
      dict[dictop] = c;
      dictop = (dictop + 1) % DICT_SIZE;
    } else {
      int patofs = reader.Read(13);
      if(!patofs) return;
      int patlen = reader.Read(4) + 3;
      for(int i = 0; i < patlen; ++i) {
        int c = dict[(patofs + i) % DICT_SIZE];
        out.write((char *)&c, 1);
        dict[dictop] = c;
        dictop = (dictop + 1) % DICT_SIZE;
      }
    }
  }
}
