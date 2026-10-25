CC = riscv64-unknown-elf-gcc
LD = riscv64-unknown-elf-ld
OBJCOPY = riscv64-unknown-elf-objcopy
NM = riscv64-unknown-elf-nm
CFLAGS = -Wall -Werror -O -std=gnu99 -mcmodel=medany -ffreestanding \
         -nostdlib -fno-common -ggdb -march=rv64gc -fno-stack-protector -fno-pie
UFLAGS = $(CFLAGS) -mno-relax -I.
QEMU = qemu-system-riscv64

UPROGS = sh hi spin badecall bufstorm badaccess memcap dumppt copyedge guardtest
OBJS = kernel/entry.o kernel/start.o kernel/console.o kernel/uart.o \
       kernel/printf.o kernel/main.o kernel/plic.o kernel/kernelvec.o \
       kernel/trampoline.o kernel/trap.o kernel/proc.o kernel/syscall.o \
       kernel/kalloc.o kernel/vm.o kernel/userimg.o

kernel/kernel: $(OBJS) kernel/kernel.ld
	$(LD) -T kernel/kernel.ld -o $@ $(OBJS)

kernel/%.o: kernel/%.c kernel/types.h kernel/riscv.h kernel/course_sid.h kernel/defs.h kernel/proc.h kernel/vm.h
	$(CC) $(CFLAGS) -Ikernel -c -o $@ $<

kernel/%.o: kernel/%.S kernel/riscv.h kernel/memlayout.h
	$(CC) $(CFLAGS) -Ikernel -c -o $@ $<

user/usys.S: user/usys.pl kernel/syscall.h
	perl user/usys.pl > $@

user-flat/user.ld: user/user.ld
	mkdir -p user-flat
	awk '{ if ($$0 ~ /^[[:space:]]*\.data[[:space:]]*:/) { print "  . = ALIGN(0x1000);"; print "  __user_text_end = .;"; print "  __user_data_start = .;" } print }' $< > $@

user-flat/%.bin: user/%.c user/user.h user/ulib.c user/printf.c user/usys.S user-flat/user.ld
	mkdir -p user-flat
	$(CC) $(UFLAGS) -Iuser -c user/ulib.c -o user-flat/ulib.o
	$(CC) $(UFLAGS) -Iuser -c user/printf.c -o user-flat/printf.o
	$(CC) $(UFLAGS) -Iuser -c user/usys.S -o user-flat/usys.o
	$(CC) $(UFLAGS) -Iuser -c $< -o user-flat/$*.o
	$(LD) --no-relax -T user-flat/user.ld -o user-flat/$*.elf user-flat/$*.o user-flat/ulib.o user-flat/printf.o user-flat/usys.o
	$(OBJCOPY) -O binary user-flat/$*.elf $@

user/badecall.c: tests/badecall.c
	cp $< $@

user/bufstorm.c: tests/bufstorm.c
	cp $< $@

user/badaccess.c: lab3-v2-20260929/support/tests/badaccess.c
	cp $< $@

user/memcap.c: lab3-v2-20260929/support/tests/memcap.c
	cp $< $@

user/guardtest.c: tests/guardtest.c
	cp $< $@

kernel/userimg.S: Makefile $(addprefix user-flat/,$(UPROGS:=.bin))
	printf '.section .rodata\n .balign 8\n .global _uprog_table\n_uprog_table:\n' > $@.tmp
	for p in $(UPROGS); do \
	  memsz=`$(NM) -n user-flat/$$p.elf | awk '$$3=="end" {print $$1}'`; \
	  text_end=`$(NM) -n user-flat/$$p.elf | awk '$$3=="__user_text_end" {print $$1}'`; \
	  data_start=`$(NM) -n user-flat/$$p.elf | awk '$$3=="__user_data_start" {print $$1}'`; \
	  test -n "$$memsz" -a -n "$$text_end" -a -n "$$data_start"; \
	  printf ' .quad _uprog_%s_start\n .quad _uprog_%s_end\n .quad 0x%s\n .quad 0x%s\n .quad 0x%s\n .asciz "%s"\n .balign 8\n' $$p $$p $$memsz $$text_end $$data_start $$p >> $@.tmp; \
	done
	printf ' .quad 0\n .quad 0\n .quad 0\n .quad 0\n .quad 0\n' >> $@.tmp
	for p in $(UPROGS); do \
	  printf ' .global _uprog_%s_start\n_uprog_%s_start:\n .incbin "user-flat/%s.bin"\n .global _uprog_%s_end\n_uprog_%s_end:\n .balign 8\n' $$p $$p $$p $$p $$p >> $@.tmp; \
	done
	mv $@.tmp $@

qemu: kernel/kernel
	$(QEMU) -machine virt -bios none -kernel kernel/kernel -m 128M -smp 1 -nographic

test: kernel/kernel
	python3 tools/inject_uart.py --tree . --script tests/lab3.script

clean:
	rm -rf kernel/*.o kernel/kernel kernel/userimg.S user/usys.S user-flat user/badecall.c user/bufstorm.c user/badaccess.c user/memcap.c user/guardtest.c

.PHONY: qemu test clean
