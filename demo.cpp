#include <cstddef>
#include <unistd.h>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <numeric>
#include "sbrk_malloc.hpp"


int main()
{
  size_t n = 1 << 10;

  // malloc something
  int *ptr = static_cast<int*>(sbrk_malloc(n * sizeof(int)));

  std::iota(ptr, ptr + n, 0);

  assert(std::is_sorted(ptr, ptr + n));

  sbrk_free(ptr);

  return 0;
}

