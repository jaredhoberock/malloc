void *sbrk_malloc(std::size_t size)
{
  void *result = 0;

  // get the current location of the break
  result = sbrk(0);

  // move the break size bytes to the right
  if(sbrk(size) == reinterpret_cast<void *>(-1))
  {
    return 0;
  }

  return result;
}

void sbrk_free(void *)
{
  // just leak it
}

