#ifndef BOOST_H
#define BOOST_H
#define BOOST_NO_EXCEPTIONS

#include <iostream>
#include <boost/assert.hpp>

namespace boost {
  template <class E>
    void throw_exception(E const& e){
      std::cout << e.what() << std::endl;
      BOOST_ASSERT(false);
    }
}
#endif
