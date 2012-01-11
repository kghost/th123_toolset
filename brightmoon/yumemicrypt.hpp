#ifndef YUMEMICRYPT_HPP
#define YUMEMICRYPT_HPP

#include <boost/iostreams/char_traits.hpp> // EOF, WOULD_BLOCK
#include <boost/iostreams/categories.hpp>  // input_filter_tag
#include <boost/iostreams/operations.hpp>  // get

struct yumemicrypt_input_filter {
  typedef char char_type;
  typedef boost::iostreams::input_filter_tag category;

  yumemicrypt_input_filter(unsigned char x) : x_(x) {}

  template<typename Source>
  int get(Source& src)
  {
    int c = boost::iostreams::get(src);
    if(c != EOF) {
      c ^= x_;
      x_ -= c;
    }
    return c;
  }

  unsigned char x_;
};

#endif
