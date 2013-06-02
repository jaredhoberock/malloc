#include <cstddef>
#include <unistd.h>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <numeric>
#include <cstdlib>
#include "first_fit_malloc.hpp"


int main()
{
  for(int i = 0; i < 100; ++i)
  {
    size_t n = std::rand() % (1 << 10);

    // malloc something
    int *ptr = static_cast<int*>(first_fit_malloc(n * sizeof(int)));

    std::iota(ptr, ptr + n, 0);

    assert(std::is_sorted(ptr, ptr + n));

    first_fit_free(ptr);
  }

  return 0;
}

