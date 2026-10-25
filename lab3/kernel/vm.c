#include "types.h"
#include "memlayout.h"
#include "riscv.h"
#include "course_sid.h"
#include "proc.h"
#include "defs.h"

#define PTE_A (1L << 6)
#define PTE_D (1L << 7)

pagetable_t kernel_pagetable;
extern char kernel_end[];
extern char trampoline[];

static void
prune_empty(pagetable_t pagetable, uint64 va)
{
  pte_t *p2 = &pagetable[PX(2, va)];
  if (!(*p2 & PTE_V) || (*p2 & (PTE_R | PTE_W | PTE_X)))
    return;
  pagetable_t level1 = (pagetable_t)PTE2PA(*p2);
  pte_t *p1 = &level1[PX(1, va)];
  if ((*p1 & PTE_V) && !(*p1 & (PTE_R | PTE_W | PTE_X))) {
    pagetable_t level0 = (pagetable_t)PTE2PA(*p1);
    int empty = 1;
    for (int i = 0; i < 512; i++)
      if (level0[i] & PTE_V) { empty = 0; break; }
    if (empty) {
      *p1 = 0;
      kfree(level0);
    }
  }
  int empty = 1;
  for (int i = 0; i < 512; i++)
    if (level1[i] & PTE_V) { empty = 0; break; }
  if (empty) {
    *p2 = 0;
    kfree(level1);
  }
}

pte_t *
walk(pagetable_t pagetable, uint64 va, int alloc)
{
  if (!pagetable || va >= MAXVA)
    return 0;

  for (int level = 2; level > 0; level--) {
    pte_t *pte = &pagetable[PX(level, va)];
    if (*pte & PTE_V) {
      if (*pte & (PTE_R | PTE_W | PTE_X))
        return 0;
    } else {
      if (!alloc)
        return 0;
      pagetable_t next = kalloc();
      if (!next)
        return 0;
      *pte = PA2PTE(next) | PTE_V;
    }
    pagetable = (pagetable_t)PTE2PA(*pte);
  }
  return &pagetable[PX(0, va)];
}

uint64
walkaddr(pagetable_t pagetable, uint64 va, int perm)
{
  pte_t *pte;
  if (va >= MAXVA || (pte = walk(pagetable, va, 0)) == 0)
    return 0;
  if (!(*pte & PTE_V) || !(*pte & PTE_U) || (*pte & perm) != perm)
    return 0;
  return PTE2PA(*pte);
}

int
mappages(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm)
{
  if (size == 0)
    return 0;
  if ((va | pa) % PGSIZE || size > MAXVA || va >= MAXVA || va + size < va ||
      PGROUNDUP(va + size) > MAXVA)
    return -1;

  uint64 first = va;
  uint64 end = PGROUNDUP(va + size);
  uint64 last_mapped = first;
  for (uint64 a = first; a < end; a += PGSIZE, pa += PGSIZE) {
    pte_t *pte = walk(pagetable, a, 1);
    if (!pte || (*pte & PTE_V)) {
      for (uint64 undo = first; undo < last_mapped; undo += PGSIZE) {
        pte_t *old = walk(pagetable, undo, 0);
        if (old)
          *old = 0;
        prune_empty(pagetable, undo);
      }
      prune_empty(pagetable, a);
      sfence_vma();
      return -1;
    }
    *pte = PA2PTE(pa) | perm | PTE_V | PTE_A;
    if (perm & PTE_W)
      *pte |= PTE_D;
    last_mapped = a + PGSIZE;
  }
  sfence_vma();
  return 0;
}

void
uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free)
{
  if (va % PGSIZE)
    panic("uvmunmap alignment");
  for (uint64 i = 0; i < npages; i++, va += PGSIZE) {
    pte_t *pte = walk(pagetable, va, 0);
    if (!pte || !(*pte & PTE_V)) {
      prune_empty(pagetable, va);
      continue;
    }
    if (!(*pte & (PTE_R | PTE_W | PTE_X)))
      panic("uvmunmap nonleaf");
    uint64 pa = PTE2PA(*pte);
    *pte = 0;
    if (do_free)
      kfree((void *)pa);
    prune_empty(pagetable, va);
  }
  sfence_vma();
}

static void
freewalk(pagetable_t pagetable)
{
  for (int i = 0; i < 512; i++) {
    pte_t pte = pagetable[i];
    if (!(pte & PTE_V))
      continue;
    pagetable[i] = 0;
    if (pte & (PTE_R | PTE_W | PTE_X)) {
      if (pte & PTE_U)
        kfree((void *)PTE2PA(pte));
    } else {
      freewalk((pagetable_t)PTE2PA(pte));
    }
  }
  kfree(pagetable);
}

void
uvmfree(pagetable_t pagetable)
{
  if (pagetable) {
    freewalk(pagetable);
    sfence_vma();
  }
}

int
uvmalloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz, int perm)
{
  if (newsz < oldsz)
    return -1;
  uint64 a = PGROUNDUP(oldsz);
  for (; a < newsz; a += PGSIZE) {
    void *mem = kalloc();
    if (!mem) {
      uvmdealloc(pagetable, a, oldsz);
      return -1;
    }
    if (mappages(pagetable, a, PGSIZE, (uint64)mem, perm) < 0) {
      kfree(mem);
      uvmdealloc(pagetable, a, oldsz);
      return -1;
    }
  }
  return 0;
}

uint64
uvmdealloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz)
{
  if (newsz >= oldsz)
    return oldsz;
  uint64 oldtop = PGROUNDUP(oldsz);
  uint64 newtop = PGROUNDUP(newsz);
  if (newtop < oldtop)
    uvmunmap(pagetable, newtop, (oldtop - newtop) / PGSIZE, 1);
  return newsz;
}

int
uvmcopy(pagetable_t old, pagetable_t new, uint64 sz)
{
  for (uint64 va = 0; va < PGROUNDUP(sz); va += PGSIZE) {
    pte_t *pte = walk(old, va, 0);
    if (!pte || !(*pte & PTE_V))
      continue;
    if (!(*pte & (PTE_R | PTE_W | PTE_X)))
      return -1;
    void *mem = kalloc();
    if (!mem)
      return -1;
    uint64 pa = PTE2PA(*pte);
    for (uint64 i = 0; i < PGSIZE; i++)
      ((uchar *)mem)[i] = ((uchar *)pa)[i];
    if (mappages(new, va, PGSIZE, (uint64)mem, PTE_FLAGS(*pte) & ~PTE_V) < 0) {
      kfree(mem);
      return -1;
    }
  }
  return 0;
}

int
copyin(pagetable_t pagetable, void *dst, uint64 srcva, uint64 len)
{
  uchar *d = dst;
  while (len) {
    uint64 pa = walkaddr(pagetable, srcva, PTE_R);
    if (!pa)
      return -1;
    uint64 n = PGSIZE - (srcva & (PGSIZE - 1));
    if (n > len)
      n = len;
    pa += srcva & (PGSIZE - 1);
    for (uint64 i = 0; i < n; i++)
      d[i] = ((uchar *)pa)[i];
    len -= n;
    d += n;
    srcva += n;
  }
  return 0;
}

int
copyout(pagetable_t pagetable, uint64 dstva, const void *src, uint64 len)
{
  const uchar *s = src;
  while (len) {
    uint64 pa = walkaddr(pagetable, dstva, PTE_W);
    if (!pa)
      return -1;
    uint64 n = PGSIZE - (dstva & (PGSIZE - 1));
    if (n > len)
      n = len;
    pa += dstva & (PGSIZE - 1);
    for (uint64 i = 0; i < n; i++)
      ((uchar *)pa)[i] = s[i];
    len -= n;
    s += n;
    dstva += n;
  }
  return 0;
}

int
copyinstr(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max)
{
  for (uint64 i = 0; i < max; i++) {
    if (copyin(pagetable, &dst[i], srcva + i, 1) < 0)
      return -1;
    if (dst[i] == 0)
      return 0;
  }
  return -1;
}

pagetable_t
kvmmake(void)
{
  pagetable_t kp = kalloc();
  if (!kp)
    return 0;
  if (mappages(kp, KERNBASE, PHYSTOP - KERNBASE, KERNBASE,
               PTE_R | PTE_W | PTE_X) < 0 ||
      mappages(kp, UART0, PGSIZE, UART0, PTE_R | PTE_W) < 0 ||
      mappages(kp, PLIC, 0x400000, PLIC, PTE_R | PTE_W) < 0 ||
      mappages(kp, TRAMPOLINE, PGSIZE, (uint64)trampoline,
               PTE_R | PTE_X) < 0) {
    uvmfree(kp);
    return 0;
  }
  return kp;
}

void
kvminit(void)
{
  kernel_pagetable = kvmmake();
  if (!kernel_pagetable)
    panic("kernel page table");
}

void
kvminithart(void)
{
  sfence_vma();
  w_satp(MAKE_SATP(kernel_pagetable));
  sfence_vma();
}

static void
dump_hex64(uint64 value)
{
  static const char digits[] = "0123456789abcdef";
  int shift = 60;
  while (shift > 0 && ((value >> shift) & 0xf) == 0)
    shift -= 4;
  for (; shift >= 0; shift -= 4)
    consputc(digits[(value >> shift) & 0xf]);
}

static void
dump_level(pagetable_t pt, int level, uint64 prefix)
{
  for (int i = 0; i < 512; i++) {
    pte_t pte = pt[i];
    if (!(pte & PTE_V))
      continue;
    uint64 va = prefix | ((uint64)i << PXSHIFT(level));
    uint64 pa = PTE2PA(pte);
    if (pte & (PTE_R | PTE_W | PTE_X)) {
      const char *r = (pte & PTE_R) ? "r" : "-";
      const char *w = (pte & PTE_W) ? "w" : "-";
      const char *x = (pte & PTE_X) ? "x" : "-";
      const char *u = (pte & PTE_U) ? "u" : "-";
      printf("LEAF va=0x"); dump_hex64(va);
      printf(" pa=0x"); dump_hex64(pa);
      printf(" perm=%s%s%s%s\n", r, w, x, u);
    } else {
      printf("L%d va=0x", level); dump_hex64(va);
      printf(" pa=0x"); dump_hex64(pa);
      printf(" perm=----\n");
      if (level > 0)
        dump_level((pagetable_t)pa, level - 1, va);
    }
  }
}

void
dump_pagetable(pagetable_t pagetable)
{
  printf("DUMP-PAGETABLE begin\n");
  dump_level(pagetable, 2, 0);
  printf("DUMP-PAGETABLE end\n");
}
