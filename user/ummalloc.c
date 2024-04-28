#include "kernel/types.h"

//
#include "user/user.h"

//
#include "ummalloc.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

void* list_top;

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define foot(p) ((char *)(p) + (*(uint *)((char *)p - 4) & ~0x7) - 8)

#define next(p) ((char *)(p) + (*(uint *)((char *)p - 4) & ~0x7))

#define prev(p) ((char *)(p) - (*(uint *)((char *)p - 8) & ~0x7))

void* merge(void* p) {
  uint pre = (*(uint *)(foot(prev(p)))) & 0x1;
  uint nex = (*(uint *)((char *)(next(p) - 4))) & 0x1;
  uint size = (*(uint *)((char *)p - 4)) & ~0x7;

  if(pre && !nex) {
    size += (*(uint *)((char *)(next(p) - 4))) & ~0x7;
    *(uint *)((char *)p - 4) = size | 0;
    *(uint *)(foot(p)) = size | 0;
  }

  else if(!pre && nex) {
    size += (*(uint *)(foot(prev(p)))) & ~0x7;
    *(uint *)((char *)prev(p) - 4) = size | 0;
    *(uint *)(foot(p)) = size | 0;
    p = prev(p);
  }

  else if(!pre && !nex) {
    size += (*(uint *)((char *)(prev(p) - 4))) & ~0x7;
    size += (*(uint *)(foot(next(p)))) & ~0x7;
    *(uint *)((char *)prev(p) - 4) = size | 0;
    *(uint *)(foot(next(p))) = size | 0;
    p = prev(p);
  }

  return p;

}

void* alloc(uint size) {
  void* temp;
  uint space;

  space = ALIGN(size);
  temp = sbrk(space);

  *(uint *)((char *)temp - 4) = space | 0;
  *(uint *)(foot(temp)) = space | 0;
  *(uint *)((char *)(next(temp) - 4)) = 1;

  return merge(temp);
}

void* find(uint size) {
  void* p;

  for(p = list_top; ((*(uint *)((char *)p - 4)) & ~0x7) > 0; p = next(p)) {
    if(((*(uint *)((char *)p - 4) & ~0x7) >= size) && ((*(uint *)((char *)p - 4) & 0x1) == 0)) {
      return p;
    }
  }

  return 0;
}

void place(void* p, uint size) {
  uint space = (*(uint *)((char *)p - 4)) & ~0x7;

  if((space - size) >= 8) {
    *(uint *)((char *)p - 4) = size | 1;
    *(uint *)(foot(p)) = size | 1;
    p = next(p);
    *(uint *)((char *)p - 4) = (space - size) | 0;
    *(uint *)(foot(p)) = (space - size) | 0;
  } else {
    *(uint *)((char *)p - 4) = space | 1;
    *(uint *)(foot(p)) = space | 1;
  }

}
/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) { 
  list_top = sbrk(16);
  *(uint*)list_top = 0;
  *(uint*)((char *)list_top + 4) = 9;
  *(uint*)((char *)list_top + 8) = 9;
  *(uint*)((char *)list_top + 12) = 1;
  list_top = (void *)((char *)list_top + 8);
  alloc(4096);
  return 0; 
  }

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(uint size) {
  uint space;
  uint extra = 4096;
  void* p;
  if(size <= 8) space = 16;
  else  space = 8 * ((size + 15) / 8);
  p = find(space);
  if(p != 0) {
    place(p,space);
    return p;
  }

  if(space > extra) {
    extra = space;
  }

  p = alloc(extra);
  place(p,space);

  return p;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
  uint size = (*(uint *)((char *)ptr - 4)) & ~0x7;
  *(uint *)((char *)ptr - 4) = size | 0;
  *(uint *)(foot(ptr)) = size | 0;
  merge(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, uint size) {
  void *oldptr = ptr;
  void *newptr;
  void* next;
  uint space;
  uint sum;
  uint extra = 4096;

  if(ptr == 0) return mm_malloc(size);
  else if (size == 0) {
    mm_free(ptr);
    return 0;
  }

  uint block = (*(uint *)((char *)ptr - 4)) & ~0x7;
  if(size <= 8) space = 16;
  else  space = 8 * ((size + 15) / 8);

  if(space == block) return ptr;
  else if(space < block) {
    place(ptr,space);
    return ptr;
  } else {
    next = next(ptr);
    sum = block + ((*(uint *)((char *)next - 4)) & ~0x7);
    if(((*(uint *)((char *)next - 4)) & 0x1) == 0 && sum >= space) {
      *(uint *)((char *)ptr - 4) = sum | 0;
      place(ptr,space);
      return ptr;
    } else {
      newptr = find(space);
      if (newptr == 0) {
        if(space > extra) {
          extra = space;
        }
        newptr = alloc(extra);
      }
      place(newptr,space);
      memcpy(newptr, oldptr, block - 8);
      mm_free(oldptr);
      return newptr;
    }
  }
}
