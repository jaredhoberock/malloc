#pragma once

// inspired by http://www.inf.udec.cl/~leo/Malloc_tutorial.pdf


// TODO introduce heap_begin & heap_end
// TODO eliminate sbrk(0) calls


struct block
{
  size_t  size;
  block  *prev;
  int     is_free;
};


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


block *find_first_free_insertion_point(size_t size)
{
  block *prev = heap_end;
  block *b = heap_begin;

  while(b != heap_end && !(b->is_free && b->size >= size))
  {
    prev = b;
    b = next(b);
  }

  return prev;
}


block *extend_heap(block *prev, size_t size)
{
  if(heap_begin == 0)
  {
    // initialize the extents of the heap to the current break
    heap_begin = reinterpret_cast<block*>(sbrk(0));
    heap_end = heap_begin;
  }

  // get the position of the current break
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


// XXX this is how malloc ought to look
//     we record the extent of the heap in [heap_begin,heap_end)
//     find_first_free needs to return the node immediately previous
//     to the first free block so that we know how to extend the heap
//     in the case that there's no free block
void *first_fit_malloc(size_t size)
{
  size_t aligned_size = align4(size);

  block *prev = find_first_free_insertion_point(aligned_size);

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


// this is how free ought to look
//void first_fit_free(void *ptr)
//{
//  if(ptr != 0)
//  {
//    block *b = get_block(ptr);
//
//    // free the block
//    b->is_free = true;
//
//    // try to fuse the freed block with the previous block
//    if(b->prev != 0 && b->prev->is_free)
//    {
//      b = b->prev;
//      fuse_block(b);
//    } // end if
//
//    // now try to fuse with the next block
//    if(next(b) != heap_end)
//    {
//      fuse_block(b);
//    } // end if
//    else
//    {
//      // we just freed the last block in the heap
//      if(b->prev == 0)
//      {
//        // the heap is empty
//        heap_begin = 0;
//        heap_end = 0;
//      } // end if
//      else
//      {
//        heap_end = b;
//      } // end else
//
//      // let the OS know where the new break is
//      brk(b);
//    } // end else
//  } // end if
//} // end free()


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
      // we just freed the last block in the heap
      if(b->prev == 0)
      {
        // the heap is empty
        heap_begin = 0;
        heap_end = 0;
      } // end if
      else
      {
        heap_end = b;
      } // end else

      // the the OS know where the new break is
      brk(b);
    } // end else
  } // end if
} // end first_fit_free()

