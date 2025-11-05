UNAME_M := $(shell uname -m)

ifeq ($(UNAME_M),aarch64)
PREFIX:=i686-linux-gnu-
BOOTIMG:=/usr/lib/grub/i386-pc/boot.img
GRUBLOC:=/usr/local/grub/bin/
else
PREFIX:=
BOOTIMG:=/usr/lib/grub/i386-pc/boot.img
GRUBLOC :=
endif

CC := $(PREFIX)gcc
LD := $(PREFIX)ld
OBJDUMP := $(PREFIX)objdump
OBJCOPY := $(PREFIX)objcopy
SIZE := $(PREFIX)size
CONFIGS := -DCONFIG_HEAP_SIZE=4096
CFLAGS := -ffreestanding -mgeneral-regs-only -mno-mmx -m32 -march=i386 -fno-pie -fno-stack-protector -g3 -Wall 

ODIR = obj
SDIR = src

OBJS := \
	rprintf.o \
	terminal.o \
	page.O \
	mmu.o \
	kernel_main.o \
	interrupt.o \
	ide.o \
	fat.o \
	malloc.o

# Make sure to keep a blank line here after OBJS list

OBJ = $(patsubst %,$(ODIR)/%,$(OBJS))

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) $(CFLAGS) -c -g -o $@ $^

$(ODIR)/%.o: $(SDIR)/%.s
	nasm -f elf32 -g -o $@ $^

all: bin rootfs.img

bin: obj $(OBJ)
	$(LD) -melf_i386  obj/* -Tkernel.ld -o kernel
	$(SIZE) kernel

obj:
	mkdir -p obj

rootfs.img:
	dd if=/dev/zero of=rootfs.img bs=1M count=32
	$(GRUBLOC)grub-mkimage -p "(hd0,msdos1)/boot" -o grub.img -O i386-pc normal biosdisk multiboot multiboot2 configfile fat exfat part_msdos
	dd if=$(BOOTIMG) of=rootfs.img conv=notrunc
	dd if=grub.img of=rootfs.img conv=notrunc seek=1
	echo 'start=2048, type=83, bootable' | sfdisk rootfs.img
	mkfs.vfat --offset 2048 -F16 rootfs.img
	mcopy -i rootfs.img@@1M kernel ::/
	mmd -i rootfs.img@@1M boot
	mcopy -i rootfs.img@@1M grub.cfg ::/boot

debug:
	./launch_qemu.sh

debug-monitor:
	screen -S qemu -d -m qemu-system-i386 -S -s -hda rootfs.img -monitor stdio
	TERM=xterm i386-unknown-elf-gdb -x gdb_os.txt && killall qemu-system-i386

clean:
	rm -f grub.img kernel rootfs.img obj/*
.PHONY: run
run: all
	@if command -v qemu-system-i386 >/dev/null 2>&1; then \
		qemu-system-i386 -m 256 -drive file=$(PWD)/rootfs.img,format=raw,if=ide -boot c -display curses; \
	else \
		qemu-system-x86_64 -cpu qemu32 -m 256 -drive file=$(PWD)/rootfs.img,format=raw,if=ide -boot c -display curses; \
	fi
