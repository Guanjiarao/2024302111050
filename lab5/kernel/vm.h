#ifndef VM_H
#define VM_H

#include "riscv.h"

#define SBRK_EAGER 1
#define SBRK_LAZY  2
#define PTE_COW  (1L << 8)
#define PTE_RSW1 (1L << 9)

extern pagetable_t kernel_pagetable;
void kinit(void);
void *kalloc(void);
void kfree(void *pa);
uint64 kfree_count(void);
void kref_inc(uint64 pa);
uint64 kref_get(uint64 pa);
uint64 kshared_count(void);
uint64 kcow_copy_count(void);
void kcow_copy_inc(void);
void kref_selftest(void);
pagetable_t kvmmake(void);
void kvminit(void);
void kvminithart(void);
void cowinit(void);
pte_t *walk(pagetable_t pagetable, uint64 va, int alloc);
uint64 walkaddr(pagetable_t pagetable, uint64 va, int perm);
int mappages(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm);
void uvmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free);
void uvmfree(pagetable_t pagetable);
int uvmcopy(pagetable_t old, pagetable_t new, uint64 sz);
int uvmalloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz, int perm);
uint64 uvmdealloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz);
int copyin(pagetable_t pagetable, void *dst, uint64 srcva, uint64 len);
int copyout(pagetable_t pagetable, uint64 dstva, const void *src, uint64 len);
int copyinstr(pagetable_t pagetable, char *dst, uint64 srcva, uint64 max);
int cow_alloc(pagetable_t pagetable, uint64 va);
void dump_pagetable(pagetable_t pagetable);

#endif
