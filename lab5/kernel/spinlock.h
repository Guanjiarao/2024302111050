#ifndef SPINLOCK_H
#define SPINLOCK_H

struct cpu;

struct spinlock {
  uint locked;
  const char *name;
  struct cpu *cpu;
};

void initlock(struct spinlock *lk, const char *name);
void acquire(struct spinlock *lk);
void release(struct spinlock *lk);
int holding(struct spinlock *lk);
void push_off(void);
void pop_off(void);

#endif
