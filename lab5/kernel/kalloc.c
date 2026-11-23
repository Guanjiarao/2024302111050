#include "types.h"
#include "memlayout.h"
#include "riscv.h"
#include "course_sid.h"
#include "spinlock.h"
#include "defs.h"

#define MAX_PHYS_PAGES ((PHYSTOP - KERNBASE) / PGSIZE)
#define BITMAP_BYTES ((MAX_PHYS_PAGES + 7) / 8)

extern char kernel_end[];

static struct {
  struct spinlock lock;
  uchar used[BITMAP_BYTES];
  uint16 refcnt[MAX_PHYS_PAGES];
  uint64 cow_copies;
} kmem;
static uint64 first_pa;
static uint64 managed_pages;
static uint64 usable_pages;
static uint64 free_pages;
static uint64 alloc_cursor;

static int
bit_get(uint64 i)
{
  return (kmem.used[i >> 3] >> (i & 7)) & 1;
}

static void
bit_set(uint64 i, int value)
{
  uchar mask = 1U << (i & 7);
  if (value)
    kmem.used[i >> 3] |= mask;
  else
    kmem.used[i >> 3] &= (uchar)~mask;
}

static uint64
ref_index(uint64 pa)
{
  if (pa < KERNBASE || pa >= PHYSTOP || pa % PGSIZE)
    panic("kref address");
  return (pa - KERNBASE) / PGSIZE;
}

void
kinit(void)
{
  initlock(&kmem.lock, "kmem");
  first_pa = PGROUNDUP((uint64)kernel_end);
  managed_pages = (PHYSTOP - first_pa) / PGSIZE;
  usable_pages = managed_pages > LAB3_MEMCAP_DELTA
                   ? managed_pages - LAB3_MEMCAP_DELTA : 0;
  free_pages = usable_pages;
  alloc_cursor = 0;
  kmem.cow_copies = 0;
  for (uint64 i = usable_pages; i < managed_pages; i++)
    bit_set(i, 1);
}

void *
kalloc(void)
{
  acquire(&kmem.lock);
  if (free_pages == 0) {
    release(&kmem.lock);
    return 0;
  }

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
      kmem.refcnt[ref_index((uint64)mem)] = 1;
      release(&kmem.lock);
      for (uint64 n = 0; n < PGSIZE; n++)
        mem[n] = 0;
      return mem;
    }
  }
  release(&kmem.lock);
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
  uint64 ri = ref_index(address);
  acquire(&kmem.lock);
  if (!bit_get(i) || kmem.refcnt[ri] < 1)
    panic("kfree: refcnt underflow");
  kmem.refcnt[ri]--;
  if (kmem.refcnt[ri] > 0) {
    release(&kmem.lock);
    return;
  }
  uchar *mem = pa;
  for (uint64 n = 0; n < PGSIZE; n++)
    mem[n] = 0x5a;
  bit_set(i, 0);
  free_pages++;
  release(&kmem.lock);
}

void
kref_inc(uint64 pa)
{
  uint64 i = ref_index(pa);
  acquire(&kmem.lock);
  if (kmem.refcnt[i] < 1 || kmem.refcnt[i] == (uint16)-1)
    panic("kref_inc");
  kmem.refcnt[i]++;
  release(&kmem.lock);
}

uint64
kref_get(uint64 pa)
{
  uint64 i = ref_index(pa);
  acquire(&kmem.lock);
  uint64 refs = kmem.refcnt[i];
  release(&kmem.lock);
  return refs;
}

uint64
kshared_count(void)
{
  uint64 count = 0;
  acquire(&kmem.lock);
  for (uint64 i = 0; i < MAX_PHYS_PAGES; i++)
    if (kmem.refcnt[i] > 1)
      count++;
  release(&kmem.lock);
  return count;
}

uint64
kcow_copy_count(void)
{
  acquire(&kmem.lock);
  uint64 count = kmem.cow_copies;
  release(&kmem.lock);
  return count;
}

void
kcow_copy_inc(void)
{
  acquire(&kmem.lock);
  kmem.cow_copies++;
  release(&kmem.lock);
}

uint64
kfree_count(void)
{
  acquire(&kmem.lock);
  uint64 count = free_pages;
  release(&kmem.lock);
  return count;
}

void
kref_selftest(void)
{
  uint64 free0 = kfree_count();
  uchar *page = kalloc();
  if (!page || kref_get((uint64)page) != 1)
    panic("kref selftest alloc");
  page[0] = 0xa5;
  kref_inc((uint64)page);
  kref_inc((uint64)page);
  if (kref_get((uint64)page) != 3 || kshared_count() != 1)
    panic("kref selftest share");
  kfree(page);
  kfree(page);
  if (kref_get((uint64)page) != 1 || page[0] != 0xa5)
    panic("kref selftest preserve");
  kfree(page);
  if (kfree_count() != free0 || kshared_count() != 0)
    panic("kref selftest reclaim");
  printf("kref selftest: PASS\n");
}
