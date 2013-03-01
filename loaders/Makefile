	
CROSS_COMPILE = arm-none-eabi-
CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
CODE=loader.c
LINKER=inc/link.ld
STARTUP=crt0.c

CFLAGS=-Wall -Iinc/ -Os -mthumb -DTHUMB -nostartfiles -fomit-frame-pointer -falign-functions=16

all: loader_f0.bin loader_f1.bin loader_f2.bin loader_f30.bin loader_f37.bin loader_f4.bin loader_l1.bin

%.bin: %.elf
	$(OBJCOPY) -O binary elf/$^ bin/$@

loader_f0.elf: $(STARTUP) $(CODE)
	$(CC) -o elf/$@ $(CFLAGS) -DSTM32F0 -mcpu=cortex-m0 -Wl,-T $(LINKER) $^
	
loader_f1.elf: $(STARTUP) $(CODE)
	$(CC) -o elf/$@ $(CFLAGS) -DSTM32F1 -DSTM32F10X_MD -mcpu=cortex-m3 -Wl,-T $(LINKER) $^
	
loader_f2.elf: $(STARTUP) $(CODE)
	$(CC) -o elf/$@ $(CFLAGS) -DSTM32F2 -mcpu=cortex-m3 -Wl,-T $(LINKER) $^
	
loader_f30.elf: $(STARTUP) $(CODE)
	$(CC) -o elf/$@ $(CFLAGS) -DSTM32F30 -mcpu=cortex-m4 -Wl,-T $(LINKER) $^
	
loader_f37.elf: $(STARTUP) $(CODE)
	$(CC) -o elf/$@ $(CFLAGS) -DSTM32F37 -mcpu=cortex-m4 -Wl,-T $(LINKER) $^
	
loader_f4.elf: $(STARTUP) $(CODE)
	$(CC) -o elf/$@ $(CFLAGS) -DSTM32F4 -mcpu=cortex-m4 -Wl,-T $(LINKER) $^
	
loader_l1.elf: $(STARTUP) $(CODE)
	$(CC) -o elf/$@ $(CFLAGS) -DSTM32L1 -mcpu=cortex-m3 -Wl,-T $(LINKER) $^

clean:
	rm -fv bin/*.bin
	rm -fv elf/*.elf