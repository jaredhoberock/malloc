#pragma once

// inspired by http://www.inf.udec.cl/~leo/Malloc_tutorial.pdf


struct block
{
  size_t  size;
  block  *prev;
  block  *next; // XXX isn't this just data(block) + size?
  int     is_free;
};


void *data(block *b)
{
  return reinterpret_cast<char*>(b) + sizeof(block);
}


void split_block(block *b, size_t size)
{
  block *new_block;

  // emplace a new block within the old one's data segment
  new_block = reinterpret_cast<block*>(reinterpret_cast<char*>(data(b)) + size);

  // the new block's size is the old block's size less the size of the split less the size of a block
  new_block->size = b->size - size - sizeof(block);

  new_block->prev = b;
  new_block->next = b->next;
  new_block->is_free = true;

  // the old block's size is the size of the split
  b->size = size;

  // link the old block to the new one
  b->next = new_block;
  if(new_block->next)
  {
    new_block->next->prev = new_block;
  } // end if
} // end split_block()


bool fuse_block(block *b)
{
  if(b->next && b->next->is_free)
  {
    b->size += sizeof(block) + b->next->size;
    b->next = b->next->next;

    if(b->next)
    {
      b->next->prev = b;
    }

    return true;
  }

  return false;
}


block *get_block(void *data)
{
  // the block metadata lives sizeof(block) bytes to the left of data
  return reinterpret_cast<block *>(reinterpret_cast<char *>(data) - sizeof(block));
}


// the base of the heap
block *base = 0;


block *find_first_free(block **last, size_t size)
{
  block *b = base;

  while(b && !(b->is_free && b->size >= size))
  {
    *last = b;
    b = b->next;
  }

  return b;
}


block *extend_heap(block *prev, size_t size)
{
  block *new_block;

  // get the position of the current break
  new_block = reinterpret_cast<block*>(sbrk(0));

  // move the break to the right to accomodate both a block and the requested allocation
  if(sbrk(sizeof(block) + size) == reinterpret_cast<void*>(-1))
  {
    // allocation failed
    return 0;
  }

  new_block->size = size;
  new_block->prev = prev;
  new_block->next = 0;
  new_block->is_free = false;

  if(prev)
  {
    prev->next = new_block;
  }

  return new_block;
}


size_t align4(size_t size)
{
  return ((((size - 1) >> 2) << 2) + 4);
}


void *first_fit_malloc(size_t size)
{
  block *b, *last;

  size_t aligned_size = align4(size);

  if(base)
  {
    // find a block
    last = base;

    b = find_first_free(&last, aligned_size);

    if(b)
    {
      // can we split?
      if((b->size - aligned_size) >= sizeof(block) + 4) // +4 for alignment
      {
        split_block(b, aligned_size);
      } // end if

      b->is_free = false;
    } // end if
    else
    {
      // nothing fits, extend the heap
      b = extend_heap(last, aligned_size);
      if(!b)
      {
        return 0;
      } // end if
    } // end else
  } // end if
  else
  {
    // first call
    base = extend_heap(0, aligned_size);
    if(!base)
    {
      return 0;
    } // end if

    b = base;
  } // end else

  return data(b);
} // end first_fit_malloc()


void first_fit_free(void *ptr)
{
  if(ptr != 0)
  {
    block *b = get_block(ptr);

    // free the block
    b->is_free = true;

    // try to fuse the freed block the previous block
    // XXX we could instead attempt to fuse with the next block if it is free
    //     this way maybe we could do without the prev pointer
    if(b->prev && b->prev->is_free)
    {
      b = b->prev;
      fuse_block(b);
    } // end if

    // now try to fuse with the next block
    if(b->next)
    {
      fuse_block(b);
    } // end if
    else
    {
      // we're at the end of the heap
      if(b->prev)
      {
        b->prev->next = 0;
      } // end if
      else
      {
        // we freed the last block
        base = 0;
      } // end else

      // the the OS know where the new break is
      brk(b);
    } // end else
  } // end if
} // end first_fit_free()

