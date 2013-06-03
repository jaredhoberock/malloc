#include <cstddef>
#include <unistd.h>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <numeric>
#include <cstdlib>
#include <utility>
#include "first_fit_malloc.hpp"


int main()
{
  for(int i = 0; i < 100; ++i)
  {
    size_t num_allocations = 50;

    void **allocations = static_cast<void**>(first_fit_malloc(num_allocations * sizeof(void*)));

    for(int j = 0; j < num_allocations; ++j)
    {
      size_t n = std::rand() % (1 << 10);

      int *ptr = static_cast<int*>(first_fit_malloc(n * sizeof(int)));

      std::iota(ptr, ptr + n, 0);

      assert(std::is_sorted(ptr, ptr + n));

      allocations[j] = ptr;
    }

    // free some of them
    for(int j = 0; j < num_allocations; ++j)
    {
      if((std::rand() % 2) == 0)
      {
        first_fit_free(allocations[j]);
        allocations[j] = 0;
      } // end if
    } // end for j

    // make some new allocations
    for(int j = 0; j < num_allocations; ++j)
    {
      if((std::rand() % 2) == 0 && allocations[j] == 0)
      {
        size_t n = std::rand() % (1 << 10);

        int *ptr = static_cast<int*>(first_fit_malloc(n * sizeof(int)));

        std::iota(ptr, ptr + n, 0);

        assert(std::is_sorted(ptr, ptr + n));

        allocations[j] = ptr;
      } // end if
    } // end for j

    // now free everything
    for(int j = 0; j < num_allocations; ++j)
    {
      first_fit_free(allocations[j]);
    } // end for j

    first_fit_free(allocations);
  } // end for i
} // end main


