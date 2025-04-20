# Toolchain prefix
CROSS = riscv64-elf-

# Tools
CC = $(CROSS)gcc
LD = $(CROSS)ld
OBJCOPY = $(CROSS)objcopy
OBJDUMP = $(CROSS)objdump

# Files
OBJS = entry.o start.o uart.o spinlock.o proc.o trap/trap.o \
       trap/trap_vec.o
TARGET = kernel
LINKER = kernel.ld

# Flags
CFLAGS = -march=rv64g -mabi=lp64 -nostdlib -fno-pic -mno-relax \
         -mcmodel=medany -Wall -Wextra -O0
LDFLAGS = -T $(LINKER)

all: $(TARGET).bin

# Compile .c and .S files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

# Link the ELF
$(TARGET).elf: $(OBJS) $(LINKER)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)

# Convert to raw binary
$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# Disassemble (optional)
dump: $(TARGET).elf
	$(OBJDUMP) -D $(TARGET).elf > $(TARGET).asm

# Run in QEMU
run: $(TARGET).bin
	qemu-system-riscv64 -machine virt \
                        -smp 4 \
                        -bios none \
                        -nographic \
                        -cpu rv64,zihintpause=true \
                        -monitor none \
                        -serial stdio \
                        -display none \
                        -kernel $(TARGET).bin

clean:
	rm -f $(shell find . -name '*.o' -o -name '*.elf' -o -name '*.bin' -o -name '*.asm')


.PHONY: all clean dump run
