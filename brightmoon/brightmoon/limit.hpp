#ifndef LIMIT_HPP
#define LIMIT_HPP

#include <boost/iostreams/char_traits.hpp> // EOF, WOULD_BLOCK
#include <boost/iostreams/categories.hpp>  // input_filter_tag
#include <boost/iostreams/operations.hpp>  // get

struct limit_input_filter {
  typedef char char_type;
  typedef boost::iostreams::input_filter_tag category;

  limit_input_filter(int limit) : limit_(limit), count(0) {}

  template<typename Source>
  int get(Source& src)
  {
    if(count > limit_) return EOF;
    ++count;
    return boost::iostreams::get(src);
  }

  int limit_;
  int count;
};

#endif
