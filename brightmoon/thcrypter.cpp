#include <iostream>
#include <vector>

bool thcrypter(std::istream &in, std::ostream &out, int size, unsigned char key, unsigned char step, int block, int limit)
{
  std::vector<char> inblk(block);
  std::vector<char> outblk(block);
  int addup;
  int i;

  addup = size % block;
  if(addup >= block / 4) addup = 0;
  addup += size % 2;
  size -= addup;

  while(size > 0 && limit > 0) {
    if(size < block) block = size;
    in.read(&inblk[0], block);
    if(in.fail()) return false;
    char *pin = &inblk[0];
    for(int j = 0; j < 2; ++j) {
      char *pout = &outblk[block - j - 1];
      for(int i = 0; i < (block - j + 1) / 2; ++i) {
        *pout = *pin++ ^ key;
        pout -= 2;
        key += step;
      }
    }
    out.write(&outblk[0], block);
    limit -= block;
    size  -= block;
  }
	size += addup;
  if(size > 0) {
    std::vector<char> restbuf(size);
    in.read(&restbuf[0], size);
    if(in.fail()) return false;
    out.write(&restbuf[0], size);
  }
  return true;
}
