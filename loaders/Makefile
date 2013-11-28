	
CROSS_COMPILE = arm-none-eabi-
CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
CODE=src/loader.c
STARTUP=src/crt0.c

CFLAGS=-Wall -Iinc/ -Os -g -nostdlib -mthumb -DTHUMB -nostartfiles -fomit-frame-pointer -falign-functions=16 -ffunction-sections -fdata-sections -fno-common
LDFLAGS=-Tinc/link.ld -Wl,--gc-sections

all: loader_f0.bin loader_f1.bin loader_f2.bin loader_f30.bin loader_f37.bin loader_f4.bin loader_l1.bin clean_elf
 
%.bin: %.elf
	$(OBJCOPY) -O binary elf/$^ bin/$@

loader_f0.elf: $(STARTUP) $(CODE) src/stm32f0xx_flash.c
	$(CC) -o elf/$@ $(CFLAGS) -DSTM32F0 -mcpu=cortex-m0 $(LDFLAGS) $^
	
loader_f1.elf: $(STARTUP) $(CODE) src/stm32f10x_flash.c
	$(CC) -o elf/$@ $(CFLAGS) -DSTM32F1 -DSTM32F10X_MD -mcpu=cortex-m3 $(LDFLAGS) $^
	
loader_f2.elf: $(STARTUP) $(CODE) src/stm32f2xx_flash.c
	$(CC) -o elf/$@ $(CFLAGS) -DSTM32F2 -mcpu=cortex-m3 $(LDFLAGS) $^
	
loader_f30.elf: $(STARTUP) $(CODE) src/stm32f30x_flash.c
	$(CC) -o elf/$@ $(CFLAGS) -DSTM32F30 -mcpu=cortex-m4 $(LDFLAGS) $^
	
loader_f37.elf: $(STARTUP) $(CODE) src/stm32f37x_flash.c
	$(CC) -o elf/$@ $(CFLAGS) -DSTM32F37 -mcpu=cortex-m4 $(LDFLAGS) $^
	
loader_f4.elf: $(STARTUP) $(CODE) src/stm32f4xx_flash.c
	$(CC) -o elf/$@ $(CFLAGS) -DSTM32F4 -mcpu=cortex-m4 $(LDFLAGS) $^
	
loader_l1.elf: $(STARTUP) $(CODE) src/stm32l1xx_flash.c
	$(CC) -o elf/$@ $(CFLAGS) -DSTM32L1 -mcpu=cortex-m3 $(LDFLAGS) $^

clean_bin:
	rm -fv bin/*.bin

clean_elf:
	rm -fv elf/*.elf

clean: clean_bin clean_elf
