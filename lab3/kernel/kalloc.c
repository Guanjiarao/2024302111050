#include "types.h"
#include "memlayout.h"
#include "riscv.h"
#include "course_sid.h"
#include "defs.h"

#define MAX_PHYS_PAGES ((PHYSTOP - KERNBASE) / PGSIZE)
#define BITMAP_BYTES ((MAX_PHYS_PAGES + 7) / 8)

extern char kernel_end[];

static uchar used[BITMAP_BYTES];
static uint64 first_pa;
static uint64 managed_pages;
static uint64 usable_pages;
static uint64 free_pages;
static uint64 alloc_cursor;

static int
bit_get(uint64 i)
{
  return (used[i >> 3] >> (i & 7)) & 1;
}

static void
bit_set(uint64 i, int value)
{
  uchar mask = 1U << (i & 7);
  if (value)
    used[i >> 3] |= mask;
  else
    used[i >> 3] &= (uchar)~mask;
}

void
kinit(void)
{
  first_pa = PGROUNDUP((uint64)kernel_end);
  managed_pages = (PHYSTOP - first_pa) / PGSIZE;
  usable_pages = managed_pages > LAB3_MEMCAP_DELTA
                   ? managed_pages - LAB3_MEMCAP_DELTA : 0;
  free_pages = usable_pages;
  alloc_cursor = 0;
  for (uint64 i = usable_pages; i < managed_pages; i++)
    bit_set(i, 1);
}

void *
kalloc(void)
{
  if (free_pages == 0)
    return 0;

  uint64 mid = usable_pages ? (usable_pages - 1) / 2 : 0;
  for (uint64 n = 0; n < usable_pages; n++) {
    uint64 step = alloc_cursor + n;
    if (step >= usable_pages)
      step -= usable_pages;
    uint64 i;
#if LAB3_ALLOC_ORDER == 0
    i = step;
#elif LAB3_ALLOC_ORDER == 1
    i = usable_pages - 1 - step;
#else
    if (step == 0)
      i = mid;
    else if (step & 1)
      i = mid + (step + 1) / 2;
    else
      i = mid - step / 2;
#endif
    if (i < usable_pages && !bit_get(i)) {
      bit_set(i, 1);
      free_pages--;
      alloc_cursor = step + 1;
      if (alloc_cursor == usable_pages)
        alloc_cursor = 0;
      uchar *mem = (uchar *)(first_pa + i * PGSIZE);
      for (uint64 n = 0; n < PGSIZE; n++)
        mem[n] = 0;
      return mem;
    }
  }
  return 0;
}

void
kfree(void *pa)
{
  uint64 address = (uint64)pa;
  if (!address || address < first_pa || address >= first_pa + usable_pages * PGSIZE ||
      address % PGSIZE)
    return;

  uint64 i = (address - first_pa) / PGSIZE;
  if (!bit_get(i))
    return;
  uchar *mem = pa;
  for (uint64 n = 0; n < PGSIZE; n++)
    mem[n] = 1;
  bit_set(i, 0);
  free_pages++;
}

uint64
kfree_count(void)
{
  return free_pages;
}
