#ifndef XOR_HPP
#define XOR_HPP

#include <boost/iostreams/char_traits.hpp> // EOF, WOULD_BLOCK
#include <boost/iostreams/categories.hpp>  // input_filter_tag
#include <boost/iostreams/operations.hpp>  // get

struct xor_input_filter {
  typedef char char_type;
  typedef boost::iostreams::input_filter_tag category;

  xor_input_filter(int x) : x_(x) {}

  template<typename Source>
  int get(Source& src)
  {
    int c = boost::iostreams::get(src);
    return c == EOF ? EOF : c ^ x_;
  }

  int x_;
};

#endif
