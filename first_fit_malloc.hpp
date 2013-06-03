#pragma once

// inspired by http://www.inf.udec.cl/~leo/Malloc_tutorial.pdf


struct block
{
  size_t  size;
  block  *prev;

  // XXX we could use the MSB of size to encode is_free
  int     is_free;
};


// XXX we shouldn't rely on this initialization
block *heap_begin = 0;
block *heap_end   = 0;


void *data(block *b)
{
  return reinterpret_cast<char*>(b) + sizeof(block);
}


block *prev(block *b)
{
  return b->prev;
}


block *next(block *b)
{
  return reinterpret_cast<block*>(reinterpret_cast<char*>(data(b)) + b->size);
}


void split_block(block *b, size_t size)
{
  block *new_block;

  // emplace a new block within the old one's data segment
  new_block = reinterpret_cast<block*>(reinterpret_cast<char*>(data(b)) + size);

  // the new block's size is the old block's size less the size of the split less the size of a block
  new_block->size = b->size - size - sizeof(block);

  new_block->prev = b;
  new_block->is_free = true;

  // the old block's size is the size of the split
  b->size = size;

  // link the old block to the new one
  if(next(new_block) != heap_end)
  {
    next(new_block)->prev = new_block;
  } // end if
} // end split_block()


bool fuse_block(block *b)
{
  if(next(b) != heap_end && next(b)->is_free)
  {
    b->size += sizeof(block) + next(b)->size;

    if(next(b) != heap_end)
    {
      next(b)->prev = b;
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


block *find_first_free_insertion_point(block *first, block *last, size_t size)
{
  block *prev = last;

  while(first != last && !(first->is_free && first->size >= size))
  {
    prev = first;
    first = next(first);
  }

  return prev;
}


block *extend_heap(block *prev, size_t size)
{
  // the new block goes at the current end of the heap
  block *new_block = heap_end;

  // move the break to the right to accomodate both a block and the requested allocation
  if(sbrk(sizeof(block) + size) == reinterpret_cast<void*>(-1))
  {
    // allocation failed
    return new_block;
  }

  // record the new end of the heap
  heap_end = reinterpret_cast<block*>(reinterpret_cast<char*>(heap_end) + sizeof(block) + size);

  new_block->size = size;
  new_block->prev = prev;
  new_block->is_free = false;

  return new_block;
}


size_t align4(size_t size)
{
  return ((((size - 1) >> 2) << 2) + 4);
}


void *first_fit_malloc(size_t size)
{
  // XXX this should be done prior to calling malloc
  if(heap_begin == 0)
  {
    // initialize the extents of the heap to the current break
    heap_begin = reinterpret_cast<block*>(sbrk(0));
    heap_end = heap_begin;
  }

  size_t aligned_size = align4(size);

  block *prev = find_first_free_insertion_point(heap_begin, heap_end, aligned_size);

  block *b;

  if(prev != heap_end && (b = next(prev)) != heap_end)
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
    b = extend_heap(prev, aligned_size);
    if(b == heap_end)
    {
      return 0;
    } // end if
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
    if(next(b) != heap_end)
    {
      fuse_block(b);
    } // end if
    else
    {
      heap_end = b;

      // the the OS know where the new break is
      brk(b);
    } // end else
  } // end if
} // end first_fit_free()

