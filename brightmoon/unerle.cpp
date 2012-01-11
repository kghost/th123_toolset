#include <iostream>

bool unerle(std::istream &in, std::ostream &out)
{
  int prev, pprev;
  if((pprev = prev = in.get()) != EOF) {
    out.put(prev);
    for(;(prev = in.get()) != EOF; pprev = prev) {
      out.put(prev);
      if(pprev == prev) {
        int count = in.get();
        if(count == EOF) return false;
        for(int i = 0; i < count; ++i) {
          out.put(prev);
        }
      }
    }
  }
  return true;
}
