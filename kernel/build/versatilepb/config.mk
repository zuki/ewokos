CPU = arm926ej-s
#QEMU_FLAGS = -cpu arm926 -M versatilepb -m 256M -nographic -display none -serial mon:stdio
QEMU_FLAGS = -cpu arm926 -M versatilepb -m 256M -serial mon:stdio
ARCH_CFLAGS = -mcpu=$(CPU) -DSOFT_DIV
ARCH=arm/v6
BSP=versatilepb
